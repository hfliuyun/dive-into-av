#include <iostream>
#include <SDL3/SDL.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

/**
 * 第21节：视频播放循环
 * 
 * 目标：结合 FFmpeg 解码与 SDL3 渲染，按正确帧率播放视频。
 * 重点：SDL3 定时器 (SDL_AddTimer) 与自定义事件 (SDL_EVENT_USER)。
 */

// 自定义刷新事件类型
static uint32_t REFRESH_EVENT = 0;

// 定时器回调函数
uint32_t sdl_refresh_timer_cb(void* userdata, SDL_TimerID timerID, uint32_t interval) {
    SDL_Event event;
    SDL_zero(event);
    event.type = REFRESH_EVENT;
    SDL_PushEvent(&event);
    return interval; 
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return -1;
    }

    const char* filename = argv[1];
    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* dec_ctx = nullptr;
    AVPacket* pkt = nullptr;
    AVFrame* frame = nullptr;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_TimerID timer_id = 0;
    int video_stream_idx = -1;
    int ret = 0;
    bool quit = false;
    bool is_eof = false;

    // 1. 初始化 FFmpeg
    if ((ret = avformat_open_input(&fmt_ctx, filename, nullptr, nullptr)) < 0) {
        SDL_Log("Could not open input file: %d", ret);
        goto end;
    }
    if ((ret = avformat_find_stream_info(fmt_ctx, nullptr)) < 0) {
        SDL_Log("Could not find stream info: %d", ret);
        goto end;
    }

    video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (video_stream_idx < 0) {
        SDL_Log("No video stream found.");
        ret = -1;
        goto end;
    }

    {
        AVStream* stream = fmt_ctx->streams[video_stream_idx];
        const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!decoder) {
            SDL_Log("Decoder not found.");
            ret = -1;
            goto end;
        }
        dec_ctx = avcodec_alloc_context3(decoder);
        if (!dec_ctx) {
            ret = AVERROR(ENOMEM);
            goto end;
        }
        avcodec_parameters_to_context(dec_ctx, stream->codecpar);
        if ((ret = avcodec_open2(dec_ctx, decoder, nullptr)) < 0) {
            SDL_Log("Could not open codec: %d", ret);
            goto end;
        }
    }

    // 2. 初始化 SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        ret = -1;
        goto end;
    }

    if (!SDL_CreateWindowAndRenderer("SDL3 Video Loop Player", dec_ctx->width, dec_ctx->height, 0, &window, &renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer Error: %s", SDL_GetError());
        ret = -1;
        goto end;
    }

    // 创建 YUV 纹理
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

    if (!texture) {
        SDL_Log("SDL_CreateTexture Error: %s", SDL_GetError());
        ret = -1;
        goto end;
    }

    // 3. 设置定时器刷新
    REFRESH_EVENT = SDL_RegisterEvents(1);
    {
        AVStream* stream = fmt_ctx->streams[video_stream_idx];
        double fps = av_q2d(stream->avg_frame_rate);
        if (fps <= 0) fps = 25.0;
        uint32_t interval = (uint32_t)(1000.0 / fps);
        timer_id = SDL_AddTimer(interval, sdl_refresh_timer_cb, nullptr);
        SDL_Log("Starting playback at %.2f fps (interval: %d ms)", fps, interval);
    }

    // 4. 事件循环
    pkt = av_packet_alloc();
    frame = av_frame_alloc();

    while (!quit) {
        SDL_Event event;
        if (SDL_WaitEvent(&event)) {
            if (event.type == REFRESH_EVENT) {
                while (true) {
                    ret = avcodec_receive_frame(dec_ctx, frame);
                    if (ret == 0) {
                        SDL_UpdateYUVTexture(texture, nullptr,
                                            frame->data[0], frame->linesize[0],
                                            frame->data[1], frame->linesize[1],
                                            frame->data[2], frame->linesize[2]);
                        SDL_RenderClear(renderer);
                        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
                        SDL_RenderPresent(renderer);
                        av_frame_unref(frame);
                        break; 
                    } else if (ret == AVERROR(EAGAIN)) {
                        if (is_eof) {
                            SDL_Log("Stream finished.");
                            quit = true;
                            break;
                        }
                        if (av_read_frame(fmt_ctx, pkt) >= 0) {
                            if (pkt->stream_index == video_stream_idx) {
                                if ((ret = avcodec_send_packet(dec_ctx, pkt)) < 0) {
                                    SDL_Log("Error sending packet to decoder: %d", ret);
                                    quit = true;
                                    break;
                                }
                            }
                            av_packet_unref(pkt);
                        } else {
                            is_eof = true;
                            avcodec_send_packet(dec_ctx, nullptr); // Flush
                        }
                    } else if (ret == AVERROR_EOF) {
                        SDL_Log("End of stream reached.");
                        quit = true;
                        break;
                    } else {
                        SDL_Log("Error during decoding: %d", ret);
                        quit = true;
                        break;
                    }
                }
            } else if (event.type == SDL_EVENT_QUIT) {
                quit = true;
            }
        }
    }

end:
    if (timer_id) SDL_RemoveTimer(timer_id);
    if (pkt) av_packet_free(&pkt);
    if (frame) av_frame_free(&frame);
    if (dec_ctx) avcodec_free_context(&dec_ctx);
    if (fmt_ctx) avformat_close_input(&fmt_ctx);
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();

    return ret < 0 ? 1 : 0;
}
