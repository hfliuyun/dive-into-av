#include "VideoThread.h"
#include <SDL3/SDL.h>

void video_thread_proc(PlayerContext* ctx) {
    SDL_Log("Video thread started.");
    PacketWrapper pw = {nullptr, 0};
    int last_serial = -1;

    while (!ctx->quit) {
        // TODO: 1. 处理暂停状态

        // TODO: 2. 从 Packet 队列获取数据
        // 提示：pop 是阻塞的

        // TODO: 3. 序列号检查与解码器 Flush
        // 核心思路：
        // a. 如果 pw.serial != ctx->seek_serial，说明是旧数据，丢弃并 continue
        // b. 如果 last_serial != pw.serial，说明是 Seek 后的第一个包，调用 avcodec_flush_buffers(ctx->v_dec_ctx)
        // c. 更新 last_serial

        // TODO: 4. 解码循环
        // 提示：
        // a. avcodec_send_packet
        // b. 内部循环 avcodec_receive_frame (av_frame_alloc)
        // c. 将解码出的 Frame 推入 Frame 队列 (注意带上序列号)
        
        // --- 占位逻辑 ---
        SDL_Delay(10); 
    }
    
    ctx->video_frame_queue.abort();
    SDL_Log("Video thread finished.");
}
