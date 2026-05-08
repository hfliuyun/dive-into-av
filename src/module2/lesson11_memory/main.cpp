#include <iostream>
#include <cstdio>

extern "C" {
#include <libavutil/buffer.h>
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

int main() {
    std::cout << "=== 引用计数演示 ===" << std::endl;

    // 1. 创建 AVBufferRef
    AVBufferRef *ref1 = av_buffer_alloc(1024);
    if (!ref1) {
        std::cerr << "分配 AVBufferRef 失败" << std::endl;
        return 1;
    }
    // 注意：直接访问 buffer->buffer->refcount 是为了演示目的
    // 实际代码中应避免直接访问 FFmpeg 内部实现细节
    std::cout << "创建 ref1: refcount=" << ref1->buffer->refcount << std::endl;

    // 2. 增加引用
    AVBufferRef *ref2 = av_buffer_ref(ref1);
    if (!ref2) {
        std::cerr << "增加引用失败" << std::endl;
        av_buffer_unref(&ref1);
        return 1;
    }
    std::cout << "创建 ref2: refcount=" << ref1->buffer->refcount << std::endl;

    AVBufferRef *ref3 = av_buffer_ref(ref1);
    if (!ref3) {
        std::cerr << "增加引用失败" << std::endl;
        av_buffer_unref(&ref2);
        av_buffer_unref(&ref1);
        return 1;
    }
    std::cout << "创建 ref3: refcount=" << ref1->buffer->refcount << std::endl;

    // 3. 减少引用
    av_buffer_unref(&ref3);
    std::cout << "释放 ref3: refcount=" << ref1->buffer->refcount << std::endl;

    av_buffer_unref(&ref2);
    std::cout << "释放 ref2: refcount=" << ref1->buffer->refcount << std::endl;

    av_buffer_unref(&ref1);
    std::cout << "释放 ref1: 内存已释放" << std::endl;

    std::cout << std::endl;
    std::cout << "=== AVFrame 引用计数演示 ===" << std::endl;

    // 4. AVFrame 引用计数演示
    AVFrame *frame1 = av_frame_alloc();
    if (!frame1) {
        std::cerr << "分配 AVFrame 失败" << std::endl;
        return 1;
    }

    // 设置 frame1 的参数
    frame1->width = 640;
    frame1->height = 360;
    frame1->format = AV_PIX_FMT_YUV420P;

    // 分配 frame1 的数据缓冲区
    if (av_frame_get_buffer(frame1, 0) < 0) {
        std::cerr << "分配 frame1 数据缓冲区失败" << std::endl;
        av_frame_free(&frame1);
        return 1;
    }
    std::cout << "创建 frame1: refcount=" << frame1->buf[0]->buffer->refcount << std::endl;

    // 增加引用
    AVFrame *frame2 = av_frame_clone(frame1);
    if (!frame2) {
        std::cerr << "克隆 frame 失败" << std::endl;
        av_frame_free(&frame1);
        return 1;
    }
    std::cout << "克隆 frame2: refcount=" << frame1->buf[0]->buffer->refcount << std::endl;

    // 减少引用
    av_frame_unref(frame2);
    std::cout << "清空 frame2: refcount=" << frame1->buf[0]->buffer->refcount << std::endl;

    av_frame_free(&frame1);
    std::cout << "释放 frame1: 内存已释放" << std::endl;

    std::cout << std::endl;
    std::cout << "=== 演示完成 ===" << std::endl;

    return 0;
}
