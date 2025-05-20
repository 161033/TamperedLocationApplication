#pragma once
#include "tools.h"
#include "decode_logic.h"
class XDecodeTask :public XThread
{
public:
    /// �򿪽�����
    bool Open(AVCodecParameters* para);

    //������������
    void Do(AVPacket* pkt) override; 

    //�߳�������
    void Main() override;

    // �̰߳�ȫ�����ص�ǰ��Ҫ��Ⱦ��AVFrame�����û�з���nullptr
    // ���ؽ����Ҫ�� XFreeFrame �ͷ�
    AVFrame* GetFrame();
private:
    std::mutex mux_;
    XDecode decode_;
    XAVPacketList pkt_list_;
    AVFrame* frame_ = nullptr;//�����洢
    bool need_view_ = false;    //�Ƿ���Ҫ��Ⱦ��ÿֻ֡��Ⱦһ�Σ�ͨ��GetFrame
};

