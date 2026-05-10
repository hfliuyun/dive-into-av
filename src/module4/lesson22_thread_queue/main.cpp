#include <iostream>
#include <thread>
#include <atomic>
#include <SDL3/SDL.h>
#include "SafeQueue.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

/**
 * 第22节：线程模型与队列设计
 * 
 * 目标：分离解封装、解码和渲染线程，提高播放器稳定性。
 * 重点：跨线程资源同步与优雅退出。
 */

static uint32_t REFRESH_EVENT = 0;
std::atomic<bool> g_quit(false);

// 队列定义
SafeQueue<AVPacket*> g_pkt_queue(100); // Packet 队列上限 100 个
SafeQueue<AVFrame*> g_frame_queue(3);  // Frame 队列上限 3 个（原始数据大，限制更严）

// 定时器回调
uint32_t sdl_refresh_timer_cb(void* userdata, SDL_TimerID timerID, uint32_t interval) {
    SDL_Event event;
    SDL_zero(event);
    event.type = REFRESH_EVENT;
    SDL_PushEvent(&event);
    return interval;
}

// 1. 解封装线程：文件 -> Packet 队列
void read_thread(AVFormatContext* fmt_ctx, int video_idx) {
    SDL_Log("Read thread started.");
    
    while (!g_quit) {
        AVPacket* pkt = av_packet_alloc();
        int ret = av_read_frame(fmt_ctx, pkt);
        if (ret >= 0) {
            if (pkt->stream_index == video_idx) {
                // push 是阻塞的，如果队列满了会等待
                if (!g_pkt_queue.push(pkt)) {
                    av_packet_free(&pkt);
                }
            } else {
                av_packet_free(&pkt);
            }
        } else {
            av_packet_free(&pkt);
            if (ret == AVERROR_EOF) {
                SDL_Log("Read thread reached EOF.");
            } else {
                SDL_Log("Read thread encountered error: %d", ret);
            }
            break; 
        }
    }
    // 任务完成或被外部打断
    g_pkt_queue.abort(); 
    SDL_Log("Read thread finished.");
}

// 2. 解码线程：Packet 队列 -> Frame 队列
void decode_thread(AVCodecContext* dec_ctx) {
    SDL_Log("Decode thread started.");
    AVPacket* pkt = nullptr;

    while (!g_quit) {
        // pop 是阻塞的，如果队列空了会等待
        if (!g_pkt_queue.pop(pkt)) break;

        if (avcodec_send_packet(dec_ctx, pkt) >= 0) {
            while (true) {
                AVFrame* frame = av_frame_alloc();
                int ret = avcodec_receive_frame(dec_ctx, frame);
                if (ret == 0) {
                    if (!g_frame_queue.push(frame)) {
                        av_frame_free(&frame);
                    }
                } else {
                    av_frame_free(&frame);
                    break;
                }
            }
        }
        av_packet_free(&pkt);
    }
    g_frame_queue.abort();
    SDL_Log("Decode thread finished.");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return -1;
    }

    const char* filename = argv[1];
    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* dec_ctx = nullptr;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_TimerID timer_id = 0;
    int video_idx = -1;

    // FFmpeg 初始化
    if (avformat_open_input(&fmt_ctx, filename, nullptr, nullptr) < 0) goto end;
    avformat_find_stream_info(fmt_ctx, nullptr);
    video_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_idx < 0) goto end;

    {
        AVStream* stream = fmt_ctx->streams[video_idx];
        const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
        dec_ctx = avcodec_alloc_context3(decoder);
        avcodec_parameters_to_context(dec_ctx, stream->codecpar);
        avcodec_open2(dec_ctx, decoder, nullptr);
    }

    // SDL 初始化
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer("SDL3 Multi-threaded Player", dec_ctx->width, dec_ctx->height, 0, &window, &renderer);
    
    {
        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, SDL_PIXELFORMAT_IYUV);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_STREAMING);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, dec_ctx->width);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, dec_ctx->height);
        SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_COLORSPACE_NUMBER, SDL_COLORSPACE_BT709_LIMITED);
        texture = SDL_CreateTextureWithProperties(renderer, props);
        SDL_DestroyProperties(props);
    }

    // 启动线程
    {
        // 显式创建线程对象以便后续 join
        std::thread r_thr(read_thread, fmt_ctx, video_idx);
        std::thread d_thr(decode_thread, dec_ctx);

        // 设置定时器
        REFRESH_EVENT = SDL_RegisterEvents(1);
        double fps = av_q2d(fmt_ctx->streams[video_idx]->avg_frame_rate);
        uint32_t interval = (uint32_t)(1000.0 / (fps > 0 ? fps : 25.0));
        timer_id = SDL_AddTimer(interval, sdl_refresh_timer_cb, nullptr);

        // 主渲染循环
        SDL_Event event;
        AVFrame* frame = nullptr;
        while (!g_quit) {
            if (SDL_WaitEvent(&event)) {
                if (event.type == REFRESH_EVENT) {
                    if (g_frame_queue.pop(frame)) {
                        SDL_UpdateYUVTexture(texture, nullptr,
                                            frame->data[0], frame->linesize[0],
                                            frame->data[1], frame->linesize[1],
                                            frame->data[2], frame->linesize[2]);
                        SDL_RenderClear(renderer);
                        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
                        SDL_RenderPresent(renderer);
                        av_frame_free(&frame); 
                    }
                } else if (event.type == SDL_EVENT_QUIT) {
                    g_quit = true;
                }
            }
        }

        // 退出流程：先标记退出，再终止队列阻塞，最后 join
        g_quit = true;
        g_pkt_queue.abort();
        g_frame_queue.abort();
        if (r_thr.joinable()) r_thr.join();
        if (d_thr.joinable()) d_thr.join();
    }

end:
    SDL_Log("Shutting down...");
    if (timer_id) SDL_RemoveTimer(timer_id);
    
    // 清空并释放队列中残余的资源（由 SafeQueue::pop 驱动）
    AVPacket* pkt = nullptr;
    while (g_pkt_queue.pop(pkt)) av_packet_free(&pkt);
    AVFrame* frame = nullptr;
    while (g_frame_queue.pop(frame)) av_frame_free(&frame);

    if (dec_ctx) avcodec_free_context(&dec_ctx);
    if (fmt_ctx) avformat_close_input(&fmt_ctx);
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
