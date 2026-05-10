#include <iostream>
#include <vector>
#include <fstream>
#include <SDL3/SDL.h>

/**
 * 第19节：SDL3 渲染基础
 * 
 * 目标：使用 SDL3 显示一张静态的 YUV420P 图像。
 * 重点：SDL3 的显式色彩空间管理。
 */

int main(int argc, char* argv[]) {
    // 1. 初始化 SDL 视频子系统
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init Error: %s", SDL_GetError());
        return -1;
    }

    const int width = 640;
    const int height = 360;
    const char* filename = "test.yuv";
    const size_t expected_size = static_cast<size_t>(width * height * 1.5);

    // 2. 创建窗口和渲染器
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!SDL_CreateWindowAndRenderer("SDL3 YUV Renderer", width, height, 0, &window, &renderer)) {
        SDL_Log("SDL_CreateWindowAndRenderer Error: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // 3. 创建 YUV 纹理 (通过 Properties 显式设置色彩空间)
    // SDL3 中纹理色彩空间在创建时通过属性指定
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, SDL_PIXELFORMAT_IYUV);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_STREAMING);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, width);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, height);
    // 这里我们显式设置为 BT.709 限制量程
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_COLORSPACE_NUMBER, SDL_COLORSPACE_BT709_LIMITED);
    
    SDL_Texture* texture = SDL_CreateTextureWithProperties(renderer, props);
    SDL_DestroyProperties(props); // 创建后即可销毁属性字典

    if (!texture) {
        SDL_Log("SDL_CreateTexture Error: %s", SDL_GetError());
        goto end;
    }

    // 5. 读取 YUV 数据
    {
        std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
        if (!ifs) {
            SDL_Log("Cannot open %s. Please generate it using ffmpeg first.", filename);
            goto end;
        }

        std::streamsize file_size = ifs.tellg();
        if (static_cast<size_t>(file_size) < expected_size) {
            SDL_Log("Error: File size (%lld) is smaller than expected (%zu).", (long long)file_size, expected_size);
            goto end;
        }
        ifs.seekg(0, std::ios::beg);

        size_t y_size = width * height;
        size_t uv_size = y_size / 4;
        std::vector<uint8_t> y_plane(y_size);
        std::vector<uint8_t> u_plane(uv_size);
        std::vector<uint8_t> v_plane(uv_size);

        if (!ifs.read(reinterpret_cast<char*>(y_plane.data()), y_size) ||
            !ifs.read(reinterpret_cast<char*>(u_plane.data()), uv_size) ||
            !ifs.read(reinterpret_cast<char*>(v_plane.data()), uv_size)) {
            SDL_Log("Error: Failed to read full YUV frame data.");
            goto end;
        }

        // 6. 更新纹理数据
        if (!SDL_UpdateYUVTexture(texture, nullptr, 
                                 y_plane.data(), width,
                                 u_plane.data(), width / 2,
                                 v_plane.data(), width / 2)) {
            SDL_Log("SDL_UpdateYUVTexture Error: %s", SDL_GetError());
            goto end;
        }
    }

    // 7. 事件循环
    {
        bool quit = false;
        SDL_Event event;
        SDL_Log("Rendering... Close the window to exit.");
        
        while (!quit) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    quit = true;
                }
            }

            // 渲染三部曲
            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, nullptr, nullptr); 
            SDL_RenderPresent(renderer);

            SDL_Delay(10); 
        }
    }

end:
    // 8. 资源释放
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
