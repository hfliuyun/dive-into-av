#include <iostream>
#include <fstream>
#include <vector>
#include <SDL3/SDL.h>

/**
 * 第20节：音频播放与 AudioStream 机制
 * 
 * 目标：读取原始 PCM 文件并通过 SDL3 的 AudioStream 播放。
 * 重点：了解 Push 模式下的缓冲控制。
 */

int main(int argc, char* argv[]) {
    // 1. 初始化 SDL 音频子系统
    if (!SDL_Init(SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return -1;
    }

    const char* filename = "test.pcm";
    const int freq = 44100;
    const int channels = 2;
    const SDL_AudioFormat format = SDL_AUDIO_S16LE;

    // 2. 配置音频规格并打开流
    // SDL3 将设备开启与流创建合并为一步
    SDL_AudioSpec spec = { format, channels, freq };
    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, nullptr);
    if (!stream) {
        SDL_Log("SDL_OpenAudioDeviceStream Error: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // 3. 必须显式恢复播放（新流默认暂停）
    if (!SDL_ResumeAudioStreamDevice(stream)) {
        SDL_Log("SDL_ResumeAudioStreamDevice Error: %s", SDL_GetError());
        goto end;
    }

    // 4. 读取并推送 PCM 数据
    {
        std::ifstream ifs(filename, std::ios::binary);
        if (!ifs) {
            SDL_Log("Cannot open %s. Please generate it using ffmpeg: \n"
                    "ffmpeg -f lavfi -i \"sine=frequency=440:duration=5\" -f s16le -ar 44100 -ac 2 test.pcm", 
                    filename);
            goto end;
        }

        const size_t chunk_size = 4096;
        std::vector<uint8_t> buffer(chunk_size);
        
        // 允许的最大缓冲区大小（例如 500ms 的数据）
        // 公式：采样率 * 每个样本字节数 * 声道数 * 秒数
        const int max_buffer_size = freq * sizeof(int16_t) * channels * 0.5;

        SDL_Log("Starting playback loop...");
        while (!ifs.eof()) {
            ifs.read(reinterpret_cast<char*>(buffer.data()), chunk_size);
            std::streamsize bytes_read = ifs.gcount();
            if (bytes_read <= 0) break;

            // 5. 节奏控制：防止推送过快导致内存溢出
            // 如果 SDL 内部缓冲区已满，我们就休眠
            while (SDL_GetAudioStreamAvailable(stream) > max_buffer_size) {
                SDL_Delay(10); 
            }

            if (!SDL_PutAudioStreamData(stream, buffer.data(), (int)bytes_read)) {
                SDL_Log("SDL_PutAudioStreamData Error: %s", SDL_GetError());
                break;
            }
        }

        // 6. 等待最后的数据播放完毕
        SDL_Log("Waiting for remaining audio to finish...");
        while (SDL_GetAudioStreamAvailable(stream) > 0) {
            SDL_Delay(10);
        }
        SDL_Log("Playback finished.");
    }

end:
    // 7. 清理资源
    if (stream) SDL_DestroyAudioStream(stream);
    SDL_Quit();

    return 0;
}
