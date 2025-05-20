#pragma once
#include "tools.h"
#include "demux_logic.h"
class XDemuxTask :public XThread
{
public:
    void Main();

    /// 打开解封装
    bool Open(std::string url,int timeout_ms = 1000);

    //复制视频参数
    std::shared_ptr<XPara> CopyVideoPara()
    {
        return demux_.CopyVideoPara();
    }

private:
    XDemux demux_;
    std::string url_;
    int timeout_ms_ = 0;//超时时间
};

