#pragma once
#include <mutex>
#include <vector>
struct AVCodecContext;
struct AVPacket;
struct AVFrame;

void PrintErr(int err);

//// 解码基类
class XCodec
{
public:
    /// 创建编解码上下文
    static AVCodecContext* Create(int codec_id,bool is_encode);

    /// 设置对象的编码器上下文 上下文传递到对象中，资源由XEncode维护
    /// 加锁 线程安全
    void set_c(AVCodecContext* c);

    /// 设置编码参数，线程安全
    bool SetOpt(const char* key, const char* val);
    bool SetOpt(const char* key, int val);

    /// 打开编码器 线程安全
    bool Open();

    //根据AVCodecContext 创建一个AVFrame，需要调用者释放av_frame_free
    AVFrame* CreateFrame();


protected:
    AVCodecContext* c_ = nullptr;  //编码器上下文
    std::mutex mux_;               //编码器上下文锁
};

