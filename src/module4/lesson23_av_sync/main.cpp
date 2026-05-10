#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
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
 * 第23节：音视频同步原理
 * 
 * 核心：视频同步到音频 (Sync to Audio Clock)。
 */

static uint32_t REFRESH_EVENT = 0;
std::atomic<bool> g_quit(false);

// 全局音频时钟（秒）
std::atomic<double> g_audio_clock(0.0);

// 队列定义
SafeQueue<AVPacket*> g_video_pkt_queue(100);
SafeQueue<AVPacket*> g_audio_pkt_queue(100);
SafeQueue<AVFrame*> g_video_frame_queue(3);

// 定时器回调
uint32_t sdl_refresh_timer_cb(void* userdata, SDL_TimerID timerID, uint32_t interval) {
    SDL_Event event;
    SDL_zero(event);
    event.type = REFRESH_EVENT;
    SDL_PushEvent(&event);
    return 0; 
}

// 1. 解封装线程
void read_thread(AVFormatContext* fmt_ctx, int video_idx, int audio_idx) {
    SDL_Log("Read thread started.");
    while (!g_quit) {
        AVPacket* pkt = av_packet_alloc();
        if (av_read_frame(fmt_ctx, pkt) >= 0) {
            if (pkt->stream_index == video_idx) {
                if (!g_video_pkt_queue.push(pkt)) av_packet_free(&pkt);
            } else if (pkt->stream_index == audio_idx) {
                if (!g_audio_pkt_queue.push(pkt)) av_packet_free(&pkt);
            } else {
                av_packet_free(&pkt);
            }
        } else {
            av_packet_free(&pkt);
            break; 
        }
    }
    g_video_pkt_queue.abort();
    g_audio_pkt_queue.abort();
    SDL_Log("Read thread finished.");
}

// 2. 音频解码与播放线程
void audio_thread(AVCodecContext* dec_ctx, AVStream* stream, SDL_AudioStream* sdl_stream) {
    SDL_Log("Audio thread started.");
    AVPacket* pkt = nullptr;
    AVFrame* frame = av_frame_alloc();
    AVFrame* res_frame = av_frame_alloc();
    SwrContext* swr_ctx = nullptr;
    
    // 强制输出规格：S16, 立体声, 44100Hz
    const int out_freq = 44100;
    const int out_channels = 2;
    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, out_channels);
    const int bytes_per_sec = out_freq * out_channels * 2; // 16bit = 2 bytes

    swr_ctx = swr_alloc();
    swr_alloc_set_opts2(&swr_ctx, 
        &out_layout, AV_SAMPLE_FMT_S16, out_freq,
        &dec_ctx->ch_layout, dec_ctx->sample_fmt, dec_ctx->sample_rate, 0, nullptr);
    swr_init(swr_ctx);

    while (!g_quit) {
        if (!g_audio_pkt_queue.pop(pkt)) break;

        if (avcodec_send_packet(dec_ctx, pkt) >= 0) {
            while (avcodec_receive_frame(dec_ctx, frame) == 0) {
                // 重采样
                int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
                res_frame->nb_samples = out_samples;
                res_frame->format = AV_SAMPLE_FMT_S16;
                av_channel_layout_copy(&res_frame->ch_layout, &out_layout);
                av_frame_get_buffer(res_frame, 0);

                swr_convert(swr_ctx, res_frame->data, out_samples, (const uint8_t**)frame->data, frame->nb_samples);

                // 推送到 SDL
                while (!g_quit && SDL_GetAudioStreamAvailable(sdl_stream) > bytes_per_sec * 0.5) { 
                    SDL_Delay(10);
                }
                if (g_quit) break;

                SDL_PutAudioStreamData(sdl_stream, res_frame->data[0], res_frame->nb_samples * out_channels * 2);
                
                // 更新音频时钟：需要扣除 SDL 内部缓冲区待播放的时长
                double pts = frame->best_effort_timestamp * av_q2d(stream->time_base);
                double buffered_duration = (double)SDL_GetAudioStreamAvailable(sdl_stream) / bytes_per_sec;
                g_audio_clock = pts - buffered_duration;

                av_frame_unref(res_frame);
                av_frame_unref(frame);
            }
        }
        av_packet_free(&pkt);
    }
    
    swr_free(&swr_ctx);
    av_frame_free(&frame);
    av_frame_free(&res_frame);
    av_channel_layout_uninit(&out_layout);
    SDL_Log("Audio thread finished.");
}

// 3. 视频解码线程
void video_decode_thread(AVCodecContext* dec_ctx) {
    SDL_Log("Video decode thread started.");
    AVPacket* pkt = nullptr;
    while (!g_quit) {
        if (!g_video_pkt_queue.pop(pkt)) break;
        if (avcodec_send_packet(dec_ctx, pkt) >= 0) {
            while (true) {
                AVFrame* frame = av_frame_alloc();
                if (avcodec_receive_frame(dec_ctx, frame) == 0) {
                    if (!g_video_frame_queue.push(frame)) av_frame_free(&frame);
                } else {
                    av_frame_free(&frame);
                    break;
                }
            }
        }
        av_packet_free(&pkt);
    }
    g_video_frame_queue.abort();
    SDL_Log("Video decode thread finished.");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return -1;
    }

    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext *v_dec_ctx = nullptr, *a_dec_ctx = nullptr;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_AudioStream* a_stream = nullptr;
    int v_idx = -1, a_idx = -1;
    SDL_AudioSpec target_spec = { SDL_AUDIO_S16LE, 2, 44100 };

    // FFmpeg Init
    if (avformat_open_input(&fmt_ctx, argv[1], nullptr, nullptr) < 0) {
        SDL_Log("Could not open input.");
        return -1;
    }
    avformat_find_stream_info(fmt_ctx, nullptr);
    v_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    a_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if (v_idx < 0 || a_idx < 0) {
        SDL_Log("Requires both video and audio streams.");
        goto end;
    }

    {
        const AVCodec* v_decoder = avcodec_find_decoder(fmt_ctx->streams[v_idx]->codecpar->codec_id);
        v_dec_ctx = avcodec_alloc_context3(v_decoder);
        avcodec_parameters_to_context(v_dec_ctx, fmt_ctx->streams[v_idx]->codecpar);
        avcodec_open2(v_dec_ctx, v_decoder, nullptr);

        const AVCodec* a_decoder = avcodec_find_decoder(fmt_ctx->streams[a_idx]->codecpar->codec_id);
        a_dec_ctx = avcodec_alloc_context3(a_decoder);
        avcodec_parameters_to_context(a_dec_ctx, fmt_ctx->streams[a_idx]->codecpar);
        avcodec_open2(a_dec_ctx, a_decoder, nullptr);
    }

    // SDL Init
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_CreateWindowAndRenderer("SDL3 A/V Sync Player", v_dec_ctx->width, v_dec_ctx->height, 0, &window, &renderer);
    
    // Texture
    {
        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, SDL_PIXELFORMAT_IYUV);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_STREAMING);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, v_dec_ctx->width);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, v_dec_ctx->height);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_COLORSPACE_NUMBER, SDL_COLORSPACE_BT709_LIMITED);
        texture = SDL_CreateTextureWithProperties(renderer, props);
        SDL_DestroyProperties(props);
    }

    // Audio Stream
    a_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &target_spec, NULL, nullptr);
    SDL_ResumeAudioStreamDevice(a_stream);

    // Start Threads
    {
        std::thread r_thr(read_thread, fmt_ctx, v_idx, a_idx);
        std::thread v_thr(video_decode_thread, v_dec_ctx);
        std::thread a_thr(audio_thread, a_dec_ctx, fmt_ctx->streams[a_idx], a_stream);

        // Sync Vars
        REFRESH_EVENT = SDL_RegisterEvents(1);
        double fps = av_q2d(fmt_ctx->streams[v_idx]->avg_frame_rate);
        double default_frame_delay = 1.0 / (fps > 0 ? fps : 25.0);
        
        SDL_AddTimer((uint32_t)(default_frame_delay * 1000), sdl_refresh_timer_cb, nullptr);

        SDL_Event event;
        AVFrame* v_frame = nullptr;
        while (!g_quit) {
            if (SDL_WaitEvent(&event)) {
                if (event.type == REFRESH_EVENT) {
                    if (g_video_frame_queue.pop(v_frame)) {
                        double pts = v_frame->best_effort_timestamp * av_q2d(fmt_ctx->streams[v_idx]->time_base);
                        double diff = pts - g_audio_clock;
                        
                        uint32_t actual_delay = (uint32_t)(default_frame_delay * 1000);
                        const double sync_threshold = 0.04;

                        if (diff > sync_threshold) {
                            actual_delay = (uint32_t)(default_frame_delay * 1000 * 2);
                        } else if (diff < -sync_threshold) {
                            actual_delay = 1; // 至少等待 1ms 保护 CPU
                        }

                        SDL_UpdateYUVTexture(texture, nullptr,
                                            v_frame->data[0], v_frame->linesize[0],
                                            v_frame->data[1], v_frame->linesize[1],
                                            v_frame->data[2], v_frame->linesize[2]);
                        SDL_RenderClear(renderer);
                        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
                        SDL_RenderPresent(renderer);
                        av_frame_free(&v_frame);

                        printf("A-V Diff: %.4f s, Clock: %.4f s\r", diff, (double)g_audio_clock);
                        fflush(stdout);

                        SDL_AddTimer(actual_delay, sdl_refresh_timer_cb, nullptr);
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
    }

end:
    if (v_dec_ctx) avcodec_free_context(&v_dec_ctx);
    if (a_dec_ctx) avcodec_free_context(&a_dec_ctx);
    if (fmt_ctx) avformat_close_input(&fmt_ctx);
    if (a_stream) SDL_DestroyAudioStream(a_stream);
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
