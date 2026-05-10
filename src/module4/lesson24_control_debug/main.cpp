#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <SDL3/SDL.h>
#include "SafeQueue.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
}

/**
 * 第24节：播放控制与播放器排障
 * 
 * 核心：实现暂停/恢复与精准跳转 (Seek)。
 * 修复：线程安全的 Flush 机制与 SDL 缓冲区清理。
 */

static uint32_t REFRESH_EVENT = 0;
std::atomic<bool> g_quit(false);
std::atomic<bool> g_paused(false);
std::atomic<double> g_seek_pos(-1.0); 

// 全局音频时钟
std::atomic<double> g_audio_clock(0.0);
std::atomic<double> g_video_pts(0.0); 

// 用于识别 Seek 后的过时数据包
std::atomic<int> g_seek_serial(0);

struct PacketWrapper {
    AVPacket* pkt;
    int serial;
};

struct FrameWrapper {
    AVFrame* frame;
    int serial;
};

SafeQueue<PacketWrapper> g_video_pkt_queue(100);
SafeQueue<PacketWrapper> g_audio_pkt_queue(100);
SafeQueue<FrameWrapper> g_video_frame_queue(3);

uint32_t sdl_refresh_timer_cb(void* userdata, SDL_TimerID timerID, uint32_t interval) {
    SDL_Event event;
    SDL_zero(event);
    event.type = REFRESH_EVENT;
    SDL_PushEvent(&event);
    return 0; 
}

// 辅助清理函数
void flush_pkt_queue(SafeQueue<PacketWrapper>& q) {
    auto items = q.flush();
    for (auto pw : items) av_packet_free(&pw.pkt);
}
void flush_frame_queue(SafeQueue<FrameWrapper>& q) {
    auto items = q.flush();
    for (auto fw : items) av_frame_free(&fw.frame);
}

// 1. 解封装线程
void read_thread(AVFormatContext* fmt_ctx, int video_idx, int audio_idx, SDL_AudioStream* a_stream) {
    SDL_Log("Read thread started.");
    while (!g_quit) {
        // 处理 Seek 请求
        if (g_seek_pos >= 0) {
            int64_t target = (int64_t)(g_seek_pos * AV_TIME_BASE);
            if (avformat_seek_file(fmt_ctx, -1, INT64_MIN, target, target, AVSEEK_FLAG_BACKWARD) >= 0) {
                g_seek_serial++; // 增加序列号
                
                flush_pkt_queue(g_video_pkt_queue);
                flush_pkt_queue(g_audio_pkt_queue);
                flush_frame_queue(g_video_frame_queue);
                
                if (a_stream) SDL_ClearAudioStream(a_stream);
                
                g_audio_clock = (double)g_seek_pos;
                g_video_pts = (double)g_seek_pos;
            }
            g_seek_pos = -1.0; 
        }

        if (g_video_pkt_queue.size() > 50 || g_audio_pkt_queue.size() > 50) {
            SDL_Delay(10);
            continue;
        }

        AVPacket* pkt = av_packet_alloc();
        if (av_read_frame(fmt_ctx, pkt) >= 0) {
            if (pkt->stream_index == video_idx) {
                if (!g_video_pkt_queue.push({pkt, g_seek_serial.load()})) av_packet_free(&pkt);
            } else if (pkt->stream_index == audio_idx) {
                if (!g_audio_pkt_queue.push({pkt, g_seek_serial.load()})) av_packet_free(&pkt);
            } else {
                av_packet_free(&pkt);
            }
        } else {
            av_packet_free(&pkt);
            SDL_Delay(10); 
        }
    }
    g_video_pkt_queue.abort();
    g_audio_pkt_queue.abort();
}

// 2. 音频线程
void audio_thread(AVCodecContext* dec_ctx, AVStream* stream, SDL_AudioStream* sdl_stream) {
    PacketWrapper pw = {nullptr, 0};
    AVFrame* frame = av_frame_alloc();
    AVFrame* res_frame = av_frame_alloc();
    SwrContext* swr_ctx = nullptr;
    int last_serial = -1;
    
    const int out_freq = 44100;
    const int out_channels = 2;
    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, out_channels);
    const int bytes_per_sec = out_freq * out_channels * 2;

    swr_ctx = swr_alloc();
    swr_alloc_set_opts2(&swr_ctx, &out_layout, AV_SAMPLE_FMT_S16, out_freq,
                        &dec_ctx->ch_layout, dec_ctx->sample_fmt, dec_ctx->sample_rate, 0, nullptr);
    swr_init(swr_ctx);

    while (!g_quit) {
        if (g_paused) { SDL_Delay(10); continue; }

        if (!g_audio_pkt_queue.pop(pw)) break;

        if (pw.serial != g_seek_serial) {
            av_packet_free(&pw.pkt);
            continue;
        }

        if (last_serial != pw.serial) {
            avcodec_flush_buffers(dec_ctx);
            last_serial = pw.serial;
        }

        if (avcodec_send_packet(dec_ctx, pw.pkt) >= 0) {
            while (avcodec_receive_frame(dec_ctx, frame) == 0) {
                int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
                res_frame->nb_samples = out_samples;
                res_frame->format = AV_SAMPLE_FMT_S16;
                av_channel_layout_copy(&res_frame->ch_layout, &out_layout);
                av_frame_get_buffer(res_frame, 0);

                swr_convert(swr_ctx, res_frame->data, out_samples, (const uint8_t**)frame->data, frame->nb_samples);

                while (!g_quit && SDL_GetAudioStreamAvailable(sdl_stream) > bytes_per_sec * 0.5) {
                    SDL_Delay(10);
                    if (g_seek_pos >= 0 || pw.serial != g_seek_serial) break; 
                }
                if (g_quit || pw.serial != g_seek_serial) {
                    av_frame_unref(res_frame);
                    av_frame_unref(frame);
                    break;
                }

                SDL_PutAudioStreamData(sdl_stream, res_frame->data[0], res_frame->nb_samples * out_channels * 2);
                
                double pts = frame->best_effort_timestamp * av_q2d(stream->time_base);
                double buffered_duration = (double)SDL_GetAudioStreamAvailable(sdl_stream) / bytes_per_sec;
                g_audio_clock = pts - buffered_duration;

                av_frame_unref(res_frame);
                av_frame_unref(frame);
            }
        }
        av_packet_free(&pw.pkt);
    }
    swr_free(&swr_ctx);
    av_frame_free(&frame);
    av_frame_free(&res_frame);
    av_channel_layout_uninit(&out_layout);
}

// 3. 视频解码线程
void video_decode_thread(AVCodecContext* dec_ctx) {
    PacketWrapper pw = {nullptr, 0};
    int last_serial = -1;
    while (!g_quit) {
        if (g_paused) { SDL_Delay(10); continue; }
        if (!g_video_pkt_queue.pop(pw)) break;

        if (pw.serial != g_seek_serial) {
            av_packet_free(&pw.pkt);
            continue;
        }

        if (last_serial != pw.serial) {
            avcodec_flush_buffers(dec_ctx);
            last_serial = pw.serial;
        }

        if (avcodec_send_packet(dec_ctx, pw.pkt) >= 0) {
            while (true) {
                AVFrame* frame = av_frame_alloc();
                if (avcodec_receive_frame(dec_ctx, frame) == 0) {
                    if (!g_video_frame_queue.push({frame, pw.serial})) av_frame_free(&frame);
                } else {
                    av_frame_free(&frame);
                    break;
                }
            }
        }
        av_packet_free(&pw.pkt);
    }
    g_video_frame_queue.abort();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_file>\nControls: Space: Pause, Left/Right: Seek +/- 5s, Q: Quit" << std::endl;
        return -1;
    }

    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext *v_dec_ctx = nullptr, *a_dec_ctx = nullptr;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_AudioStream* a_stream = nullptr;
    int v_idx = -1, a_idx = -1;

    if (avformat_open_input(&fmt_ctx, argv[1], nullptr, nullptr) < 0) return -1;
    avformat_find_stream_info(fmt_ctx, nullptr);
    v_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    a_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    auto setup_codec = [&](int idx, AVCodecContext** ctx) {
        const AVCodec* decoder = avcodec_find_decoder(fmt_ctx->streams[idx]->codecpar->codec_id);
        *ctx = avcodec_alloc_context3(decoder);
        avcodec_parameters_to_context(*ctx, fmt_ctx->streams[idx]->codecpar);
        avcodec_open2(*ctx, decoder, nullptr);
    };
    setup_codec(v_idx, &v_dec_ctx);
    setup_codec(a_idx, &a_dec_ctx);

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_CreateWindowAndRenderer("SDL3 Control Player", v_dec_ctx->width, v_dec_ctx->height, 0, &window, &renderer);
    
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, SDL_PIXELFORMAT_IYUV);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_STREAMING);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, v_dec_ctx->width);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, v_dec_ctx->height);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_COLORSPACE_NUMBER, SDL_COLORSPACE_BT709_LIMITED);
    texture = SDL_CreateTextureWithProperties(renderer, props);
    SDL_DestroyProperties(props);

    SDL_AudioSpec target_spec = { SDL_AUDIO_S16LE, 2, 44100 };
    a_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &target_spec, NULL, nullptr);
    SDL_ResumeAudioStreamDevice(a_stream);

    std::thread r_thr(read_thread, fmt_ctx, v_idx, a_idx, a_stream);
    std::thread v_thr(video_decode_thread, v_dec_ctx);
    std::thread a_thr(audio_thread, a_dec_ctx, fmt_ctx->streams[a_idx], a_stream);

    REFRESH_EVENT = SDL_RegisterEvents(1);
    double fps = av_q2d(fmt_ctx->streams[v_idx]->avg_frame_rate);
    double default_delay = 1.0 / (fps > 0 ? fps : 25.0);
    SDL_AddTimer((uint32_t)(default_delay * 1000), sdl_refresh_timer_cb, nullptr);

    SDL_Event event;
    FrameWrapper fw = {nullptr, 0};
    while (!g_quit) {
        if (SDL_WaitEvent(&event)) {
            if (event.type == REFRESH_EVENT) {
                if (g_paused) continue;

                if (g_video_frame_queue.pop(fw)) {
                    if (fw.serial != g_seek_serial) {
                        av_frame_free(&fw.frame);
                        SDL_Event refresh; SDL_zero(refresh); refresh.type = REFRESH_EVENT; SDL_PushEvent(&refresh);
                        continue;
                    }

                    double pts = fw.frame->best_effort_timestamp * av_q2d(fmt_ctx->streams[v_idx]->time_base);
                    g_video_pts = pts;
                    double diff = pts - g_audio_clock;
                    uint32_t next_delay = (uint32_t)(default_delay * 1000);
                    if (diff > 0.04) next_delay *= 2;
                    else if (diff < -0.04) next_delay = 1;

                    SDL_UpdateYUVTexture(texture, nullptr, fw.frame->data[0], fw.frame->linesize[0],
                                        fw.frame->data[1], fw.frame->linesize[1], fw.frame->data[2], fw.frame->linesize[2]);
                    SDL_RenderClear(renderer);
                    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
                    SDL_RenderPresent(renderer);
                    av_frame_free(&fw.frame);
                    
                    printf("PTS: %.2f, Diff: %.4f s\r", (double)g_video_pts, diff);
                    fflush(stdout);
                    SDL_AddTimer(next_delay, sdl_refresh_timer_cb, nullptr);
                } else {
                    SDL_AddTimer(10, sdl_refresh_timer_cb, nullptr); 
                }
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                    case SDLK_SPACE: 
                        g_paused = !g_paused; 
                        SDL_Log(g_paused ? "Paused" : "Resumed");
                        if (!g_paused) { 
                            SDL_Event refresh; SDL_zero(refresh); refresh.type = REFRESH_EVENT; SDL_PushEvent(&refresh);
                        }
                        break;
                    case SDLK_LEFT:  g_seek_pos = (double)g_video_pts - 5.0; break;
                    case SDLK_RIGHT: g_seek_pos = (double)g_video_pts + 5.0; break;
                    case SDLK_Q: case SDLK_ESCAPE: g_quit = true; break;
                }
            } else if (event.type == SDL_EVENT_QUIT) {
                g_quit = true;
            }
        }
    }

    g_quit = true;
    g_video_pkt_queue.abort();
    g_audio_pkt_queue.abort();
    g_video_frame_queue.abort();
    if (r_thr.joinable()) r_thr.join();
    if (v_thr.joinable()) v_thr.join();
    if (a_thr.joinable()) a_thr.join();

    avcodec_free_context(&v_dec_ctx);
    avcodec_free_context(&a_dec_ctx);
    avformat_close_input(&fmt_ctx);
    SDL_DestroyAudioStream(a_stream);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
