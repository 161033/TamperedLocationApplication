#pragma once
#include "format.h"
class XDemux :public XFormat
{
public:

    /// �򿪽��װ
    static AVFormatContext* Open(const char* url);

    /// ��ȡһ֡����
    bool Read(AVPacket* pkt);

    //����ָ��pts��
    bool Seek(long long pts,int stream_index);

};

