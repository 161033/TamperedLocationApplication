#include "format.h"
#include <iostream>
#include <thread>
#include "tools.h"
using namespace std;

extern "C" 
{
    #include <libavformat/avformat.h>
}

#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
using namespace std;

static int TimeoutCallback(void* para)
{
    auto xf = (XFormat*)para;
    if (xf->IsTimeout())return 1;//超时退出Read
    return 0; //正常阻塞
}

void XFormat::set_c(AVFormatContext* c)
{
    unique_lock<mutex> lock(mux_);
    if (c_) //清理原值
    {
        if (c_->oformat) //输出上下文
        {
            if (c_->pb)
                avio_closep(&c_->pb);
            avformat_free_context(c_);
        }
        else if (c_->iformat)  //输入上下文
        {
            avformat_close_input(&c_);
        }
        else
        {
            avformat_free_context(c_);
        }
    }
    c_ = c;
    if (!c_)
    {
        is_connected_ = false;
        return;
    }
    is_connected_ = true;

    //计时 用于超时判断
    last_time_ = NowMs();

    //设定超时处理回调
    if (time_out_ms_ > 0)
    {
        AVIOInterruptCB cb = { TimeoutCallback ,this };
        c_->interrupt_callback = cb;
    }

    //用于区分是否有音频或者视频流
    audio_index_ = -1; 
    video_index_ = -1;
    //区分音视频stream 索引
    for (int i = 0; i < c->nb_streams; i++)
    {
        //音频
        if (c->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audio_index_ = i;
            audio_time_base_.den = c->streams[i]->time_base.den;
            audio_time_base_.num = c->streams[i]->time_base.num;
        }
        else if (c->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_index_ = i;
            video_time_base_.den = c->streams[i]->time_base.den;
            video_time_base_.num = c->streams[i]->time_base.num;
            video_codec_id_ = c->streams[i]->codecpar->codec_id;
        }
    }
}
std::shared_ptr<XPara> XFormat::CopyVideoPara()
{
    int index = video_index();
    shared_ptr<XPara> re;
    unique_lock<mutex> lock(mux_);
    if (index < 0 || !c_)return re;

    re.reset(XPara::Create());
    *re->time_base = c_->streams[index]->time_base;
    avcodec_parameters_copy(re->para, c_->streams[index]->codecpar);
    return re;
}
bool XFormat::CopyPara(int stream_index, AVCodecContext* dts)
{
    unique_lock<mutex> lock(mux_);
    if (!c_)
    {
        return false;
    }
    if (stream_index<0 || stream_index>c_->nb_streams)
        return false;
    auto re = avcodec_parameters_to_context(dts, c_->streams[stream_index]->codecpar);
    if (re < 0)
    {
        return false;
    }
    return true;
}

bool XFormat::CopyPara(int stream_index, AVCodecParameters* dst)
{
    unique_lock<mutex> lock(mux_);
    if (!c_)
    {
        return false;
    }
    if (stream_index<0 || stream_index>c_->nb_streams)
        return false;
    auto re = avcodec_parameters_copy(dst, c_->streams[stream_index]->codecpar);
    if (re < 0)
    {
        return false;
    }


    return true;
}

bool XFormat::RescaleTime(AVPacket* pkt, long long offset_pts, XRational time_base)
{
    unique_lock<mutex> lock(mux_);
    if (!c_)return false;        
    auto out_stream = c_->streams[pkt->stream_index];
    AVRational in_time_base;
    in_time_base.num = time_base.num;
    in_time_base.den = time_base.den;
    pkt->pts = av_rescale_q_rnd(pkt->pts- offset_pts, in_time_base,
            out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)
        );
    pkt->dts = av_rescale_q_rnd(pkt->dts- offset_pts, in_time_base,
        out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX)
    );
    pkt->duration = av_rescale_q(pkt->duration, in_time_base, out_stream->time_base);
    pkt->pos = -1;

    return true;
}

//设定超时时间
void XFormat::set_time_out_ms(int ms)
{
    unique_lock<mutex> lock(mux_);
    this->time_out_ms_ = ms;
    //设置回调函数，处理超时退出
    if (c_)
    {
        AVIOInterruptCB cb = { TimeoutCallback ,this };
        c_->interrupt_callback = cb;
    }
}