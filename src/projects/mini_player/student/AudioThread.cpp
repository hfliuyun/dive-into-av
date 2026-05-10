#include "AudioThread.h"
#include <SDL3/SDL.h>

extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
}

void audio_thread_proc(PlayerContext* ctx) {
    SDL_Log("Audio thread started.");
    
    // TODO: 1. 初始化重采样上下文 (SwrContext)
    // 提示：强制输出为 S16LE, Stereo, 44100Hz 以对齐 SDL3 配置

    while (!ctx->quit) {
        // TODO: 2. 处理暂停状态

        // TODO: 3. 序列号检查与解码器 Flush (逻辑同视频线程)

        // TODO: 4. 解码、重采样并推送到 SDL
        // 核心思路：
        // a. avcodec_receive_frame
        // b. swr_convert
        // c. SDL_GetAudioStreamAvailable 节奏控制
        // d. SDL_PutAudioStreamData 推送数据
        // e. 更新 ctx->audio_clock (记得扣除 SDL 缓冲延迟！)
        
        // --- 占位逻辑 ---
        SDL_Delay(10); 
    }

    SDL_Log("Audio thread finished.");
}
