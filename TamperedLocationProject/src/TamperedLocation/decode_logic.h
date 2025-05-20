#pragma once
#include "codec.h"
struct AVBufferRef;
class XDecode :public XCodec
{
public:
    bool Send(const AVPacket* pkt);  //发送解码
    bool Recv(AVFrame* frame);       //获取解码
    std::vector<AVFrame*> End();    //获取缓存

    //// 初始化硬件加速
    bool InitHW(int type = 4);

};

