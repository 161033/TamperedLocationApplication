#pragma once
#include "format.h"
class XDemux :public XFormat
{
public:

    /// 打开解封装
    static AVFormatContext* Open(const char* url);

    /// 读取一帧数据
    bool Read(AVPacket* pkt);

    //调到指定pts处
    bool Seek(long long pts,int stream_index);

};

