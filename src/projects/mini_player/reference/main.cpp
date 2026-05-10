#include <iostream>
#include <thread>
#include <chrono>
#include <SDL3/SDL.h>
#include "PlayerContext.h"
#include "DemuxThread.h"
#include "VideoThread.h"
#include "AudioThread.h"

extern "C" {
#include <libavutil/time.h>
#include <libavutil/pixdesc.h>
}

/**
 * 阶段项目二：mini_player 参考实现
 * 
 * 改进：
 * 1. 修正了暂停时的音频延迟问题。
 * 2. 补全了退出时的内存清理逻辑。
 * 3. 增强了视频格式校验。
 */

#include <csignal>

// 全局指针用于信号处理
static PlayerContext* g_ctx_ptr = nullptr;
void signal_handler(int sig) {
    if (g_ctx_ptr) {
        g_ctx_ptr->quit = true;
        // 推送一个 SDL 事件以打破 SDL_WaitEvent 的阻塞
        SDL_Event event;
        SDL_zero(event);
        event.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&event);
    }
}

uint32_t sdl_refresh_timer_cb(void* userdata, SDL_TimerID timerID, uint32_t interval) {
    PlayerContext* ctx = (PlayerContext*)userdata;
    if (ctx->quit) return 0;
    SDL_Event event;
    SDL_zero(event);
    event.type = ctx->refresh_event;
    SDL_PushEvent(&event);
    return 0; 
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_file>\n"
                  << "Controls:\n"
                  << "  Space: Pause/Resume\n"
                  << "  Left/Right: Seek +/- 5s\n"
                  << "  Q/Esc: Quit" << std::endl;
        return -1;
    }

    PlayerContext ctx;
    g_ctx_ptr = &ctx;
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // 1. FFmpeg 初始化
    ctx.fmt_ctx = avformat_alloc_context();
    ctx.fmt_ctx->interrupt_callback.callback = PlayerContext::interrupt_cb;
    ctx.fmt_ctx->interrupt_callback.opaque = &ctx;

    if (avformat_open_input(&ctx.fmt_ctx, argv[1], nullptr, nullptr) < 0) return -1;
    avformat_find_stream_info(ctx.fmt_ctx, nullptr);
    ctx.video_idx = av_find_best_stream(ctx.fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    ctx.audio_idx = av_find_best_stream(ctx.fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if (ctx.video_idx < 0) {
        std::cerr << "No video stream found." << std::endl;
        return -1;
    }

    auto setup_codec = [&](int idx, AVCodecContext** c) {
        if (idx < 0) return;
        const AVCodec* d = avcodec_find_decoder(ctx.fmt_ctx->streams[idx]->codecpar->codec_id);
        *c = avcodec_alloc_context3(d);
        avcodec_parameters_to_context(*c, ctx.fmt_ctx->streams[idx]->codecpar);
        avcodec_open2(*c, d, nullptr);
    };
    setup_codec(ctx.video_idx, &ctx.v_dec_ctx);
    setup_codec(ctx.audio_idx, &ctx.a_dec_ctx);

    // 格式校验：仅支持 YUV420P
    if (ctx.v_dec_ctx->pix_fmt != AV_PIX_FMT_YUV420P) {
        std::cerr << "Error: Only YUV420P is supported. Current: " 
                  << av_get_pix_fmt_name(ctx.v_dec_ctx->pix_fmt) << std::endl;
        // 实际开发中此处应使用 sws_scale 转换，本项目为保持精简暂不支持
    }

    // 2. SDL 初始化
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_CreateWindowAndRenderer("mini_player", ctx.v_dec_ctx->width, ctx.v_dec_ctx->height, 0, &ctx.window, &ctx.renderer);
    
    {
        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, SDL_PIXELFORMAT_IYUV);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_STREAMING);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, ctx.v_dec_ctx->width);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, ctx.v_dec_ctx->height);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_COLORSPACE_NUMBER, SDL_COLORSPACE_BT709_LIMITED);
        ctx.texture = SDL_CreateTextureWithProperties(ctx.renderer, props);
        SDL_DestroyProperties(props);
    }

    SDL_AudioSpec target_spec = { SDL_AUDIO_S16LE, 2, 44100 };
    ctx.a_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &target_spec, NULL, nullptr);
    SDL_ResumeAudioStreamDevice(ctx.a_stream);

    // 3. 启动线程
    std::thread demux_thr(demux_thread_proc, &ctx);
    std::thread video_thr(video_thread_proc, &ctx);
    std::thread audio_thr(audio_thread_proc, &ctx);

    // 4. 定时器
    ctx.refresh_event = SDL_RegisterEvents(1);
    double fps = av_q2d(ctx.fmt_ctx->streams[ctx.video_idx]->avg_frame_rate);
    ctx.frame_delay = 1.0 / (fps > 0 ? fps : 25.0);
    SDL_AddTimer((uint32_t)(ctx.frame_delay * 1000), sdl_refresh_timer_cb, &ctx);

    // 5. 事件循环
    SDL_Event event;
    FrameWrapper fw = {nullptr, 0};
    while (!ctx.quit) {
        if (SDL_WaitEvent(&event)) {
            if (event.type == ctx.refresh_event) {
                if (ctx.paused) {
                    SDL_AddTimer(40, sdl_refresh_timer_cb, &ctx);
                    continue;
                }
                if (ctx.video_frame_queue.pop(fw)) {
                    if (fw.serial != ctx.seek_serial) {
                        av_frame_free(&fw.frame);
                        SDL_Event refresh; SDL_zero(refresh); refresh.type = ctx.refresh_event; SDL_PushEvent(&refresh);
                        continue;
                    }

                    double pts = fw.frame->best_effort_timestamp * av_q2d(ctx.fmt_ctx->streams[ctx.video_idx]->time_base);
                    ctx.video_pts = pts;
                    double diff = pts - ctx.audio_clock;
                    uint32_t next_delay = (uint32_t)(ctx.frame_delay * 1000);
                    if (diff > 0.04) next_delay *= 2;
                    else if (diff < -0.04) next_delay = 1;

                    SDL_UpdateYUVTexture(ctx.texture, nullptr, fw.frame->data[0], fw.frame->linesize[0],
                                        fw.frame->data[1], fw.frame->linesize[1], fw.frame->data[2], fw.frame->linesize[2]);
                    SDL_RenderClear(ctx.renderer);
                    SDL_RenderTexture(ctx.renderer, ctx.texture, nullptr, nullptr);
                    SDL_RenderPresent(ctx.renderer);
                    av_frame_free(&fw.frame);
                    
                    printf("PTS: %.2f, Diff: %.4f s\r", (double)ctx.video_pts, diff);
                    fflush(stdout);
                    if (!ctx.quit) SDL_AddTimer(next_delay, sdl_refresh_timer_cb, &ctx);
                } else {
                    if (!ctx.quit) SDL_AddTimer(10, sdl_refresh_timer_cb, &ctx); 
                }
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                    case SDLK_SPACE: 
                        ctx.paused = !ctx.paused; 
                        if (ctx.paused) SDL_PauseAudioStreamDevice(ctx.a_stream);
                        else {
                            SDL_ResumeAudioStreamDevice(ctx.a_stream);
                            SDL_Event refresh; SDL_zero(refresh); refresh.type = ctx.refresh_event; SDL_PushEvent(&refresh);
                        }
                        break;
                    case SDLK_LEFT:  ctx.seek_pos = (double)ctx.video_pts - 5.0; break;
                    case SDLK_RIGHT: ctx.seek_pos = (double)ctx.video_pts + 5.0; break;
                    case SDLK_Q: case SDLK_ESCAPE: ctx.quit = true; break;
                }
            } else if (event.type == SDL_EVENT_QUIT) {
                ctx.quit = true;
            }
        }
    }

    // 6. 清理
    ctx.quit = true;
    ctx.video_pkt_queue.abort();
    ctx.audio_pkt_queue.abort();
    ctx.video_frame_queue.abort();
    if (demux_thr.joinable()) demux_thr.join();
    if (video_thr.joinable()) video_thr.join();
    if (audio_thr.joinable()) audio_thr.join();

    // 清理队列残留资源
    PacketWrapper pw;
    while (ctx.video_pkt_queue.pop(pw)) av_packet_free(&pw.pkt);
    while (ctx.audio_pkt_queue.pop(pw)) av_packet_free(&pw.pkt);
    while (ctx.video_frame_queue.pop(fw)) av_frame_free(&fw.frame);

    avcodec_free_context(&ctx.v_dec_ctx);
    avcodec_free_context(&ctx.a_dec_ctx);
    avformat_close_input(&ctx.fmt_ctx);
    SDL_DestroyAudioStream(ctx.a_stream);
    SDL_DestroyTexture(ctx.texture);
    SDL_DestroyRenderer(ctx.renderer);
    SDL_DestroyWindow(ctx.window);
    SDL_Quit();
    return 0;
}
