#include <iostream>
#include <cstdio>
#include <string>

extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
}

void leak_memory() {
    std::cout << "故意泄漏内存..." << std::endl;

    // 故意不释放 frame
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "分配 AVFrame 失败" << std::endl;
        return;
    }
    std::cout << "分配了 AVFrame，但没有释放" << std::endl;
    // 故意不调用 av_frame_free(&frame)
}

void leak_packet() {
    std::cout << "故意泄漏 packet..." << std::endl;

    // 故意不释放 packet
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "分配 AVPacket 失败" << std::endl;
        return;
    }
    std::cout << "分配了 AVPacket，但没有释放" << std::endl;
    // 故意不调用 av_packet_free(&packet)
}

void no_leak() {
    std::cout << "正确释放内存..." << std::endl;

    AVFrame *frame = av_frame_alloc();
    if (frame) {
        std::cout << "分配了 AVFrame" << std::endl;
        av_frame_free(&frame);
        std::cout << "释放了 AVFrame" << std::endl;
    }

    AVPacket *packet = av_packet_alloc();
    if (packet) {
        std::cout << "分配了 AVPacket" << std::endl;
        av_packet_free(&packet);
        std::cout << "释放了 AVPacket" << std::endl;
    }
}

void double_free() {
    std::cout << "故意重复释放..." << std::endl;

    AVFrame *frame = av_frame_alloc();
    if (frame) {
        std::cout << "分配了 AVFrame" << std::endl;
        av_frame_free(&frame);
        std::cout << "第一次释放了 AVFrame" << std::endl;

        // 故意重复释放
        av_frame_free(&frame);
        std::cout << "第二次释放了 AVFrame（这行可能不会执行）" << std::endl;
    }
}

void use_after_free() {
    std::cout << "故意使用已释放的内存..." << std::endl;

    AVFrame *frame = av_frame_alloc();
    if (frame) {
        std::cout << "分配了 AVFrame" << std::endl;
        av_frame_free(&frame);
        std::cout << "释放了 AVFrame" << std::endl;

        // 故意使用已释放的内存
        std::cout << "访问已释放的 frame->width=" << frame->width << std::endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "用法: " << argv[0] << " <leak|leak_packet|no_leak|double_free|use_after_free>" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "leak") {
        leak_memory();
    } else if (mode == "leak_packet") {
        leak_packet();
    } else if (mode == "no_leak") {
        no_leak();
    } else if (mode == "double_free") {
        double_free();
    } else if (mode == "use_after_free") {
        use_after_free();
    } else {
        std::cerr << "未知模式: " << mode << std::endl;
        std::cerr << "可用模式: leak, leak_packet, no_leak, double_free, use_after_free" << std::endl;
        return 1;
    }

    std::cout << "程序结束" << std::endl;
    return 0;
}
