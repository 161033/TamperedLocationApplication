#pragma once
#include "tools.h"
#include "decode_logic.h"
class XDecodeTask :public XThread
{
public:
    /// 打开解码器
    bool Open(AVCodecParameters* para);

    //责任链处理函数
    void Do(AVPacket* pkt) override; 

    //线程主函数
    void Main() override;

    // 线程安全，返回当前需要渲染的AVFrame，如果没有返回nullptr
    // 返回结果需要用 XFreeFrame 释放
    AVFrame* GetFrame();
private:
    std::mutex mux_;
    XDecode decode_;
    XAVPacketList pkt_list_;
    AVFrame* frame_ = nullptr;//解码后存储
    bool need_view_ = false;    //是否需要渲染，每帧只渲染一次，通过GetFrame
};

