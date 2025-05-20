#pragma once
#include "codec.h"
struct AVBufferRef;
class XDecode :public XCodec
{
public:
    bool Send(const AVPacket* pkt);  //���ͽ���
    bool Recv(AVFrame* frame);       //��ȡ����
    std::vector<AVFrame*> End();    //��ȡ����

    //// ��ʼ��Ӳ������
    bool InitHW(int type = 4);

};

