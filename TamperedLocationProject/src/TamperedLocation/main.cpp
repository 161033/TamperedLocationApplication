#include <iostream>
#include <cstdio>
#include <thread>
#include "tools.h"
#include "demux_task.h"
#include "decode_task.h"
#include "threadpool.h"
#include "Inference.h"
#include <fstream>

using namespace std;
extern "C"
{
#include <libavformat/avformat.h>
}

const std::string DetectVideo = "test.mp4";
const std::string ResultFile = "ResultFile.log";
const int BATCH_SIZE = 250;

int main(int argc, char* argv[])
{
    XDemuxTask demux_task;
    for (;;)
    {
        if (demux_task.Open(DetectVideo))
        {
            break;
        }
        MSleep(100);
        continue;
    }
    auto para = demux_task.CopyVideoPara();


    //创建线程池
    ThreadPool pool;
    pool.setMode(PoolMode::MODE_CACHED);
    pool.start(8);
    XDecodeTask decode_task;
    if (!decode_task.Open(para->para))
    {
        LOGERROR("open decode failed!");
    }
    else
    {
        //设定一下个责任
        demux_task.set_next(&decode_task);
        demux_task.Start();
        decode_task.Start();
    }
    std::vector<std::future<Result>> futures;
    futures.reserve(BATCH_SIZE);
    std::ofstream output_file(ResultFile, ios::out | ios::app);
    if (!output_file)
    {
        LOGERROR("open result file failed!");
        return;
    }

    Inference inference;
    inference.Init();
    for (;;)
    {
        auto f = decode_task.GetFrame();
        if (!f || pool.GettaskSize() >= pool.GetMaxtaskSize())
        {
            MSleep(1);
            continue;
        }
        inference.Preprocess(f);
        int64_t pts = f->pts;
        futures.emplace_back(pool.submitTask([&inference, &pts]() {return inference.StartInference(pts); }));
        XFreeFrame(&f);
        if (futures.size()>= BATCH_SIZE)
        {
            for (auto &f : futures)
            {
                Result res = f.get();
                output_file
                    << "PTS: " << res.pts
                    << "AUC: " << res.AUC
                    << "F1: " << res.F1 << " ";
            }
            futures.clear();
            futures.reserve(BATCH_SIZE);
        }
    }
    getchar();
    return 0;
}

