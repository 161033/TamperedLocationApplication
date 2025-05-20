#include "Inference.h"
#include <iostream>
#include <cstdio>
#include <thread>
#include <opencv2/opencv.hpp>
#include "tools.h"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

const std::string model = "MSMAENET.onnx";
const int input_resize = 256;

Inference::Inference()
{
}

void Inference::Init()
{
    // 初始化 ONNX Runtime 环境
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "VideoAuthenticityDetector");
    Ort::SessionOptions session_options;
    session = Ort::Session::Create(env, model, session_options);

    // 获取输入输出信息
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::TypeInfo input_type_info = session.GetInputTypeInfo(0);
    auto input_tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
    input_shape = input_tensor_info.GetShape();

}

void Inference::Preprocess(AVFrame* frame)
{
    // 检查输入有效性
    if (!frame || !frame->data[0]) return;

    // 创建转换上下文 (YUV420P -> BGR24)
    SwsContext* sws_ctx = sws_getCachedContext(
        nullptr,
        frame->width,
        frame->height,
        (AVPixelFormat)frame->format,
        frame->width,
        frame->height,
        AV_PIX_FMT_BGR24,
        SWS_BILINEAR,
        nullptr, nullptr, nullptr
    );
    if (!sws_ctx) {
        std::cerr << "Failed to create SwsContext" << std::endl;
        return ;
    }

    cv::Mat mat_frame(frame->height, frame->width, CV_8UC3);
    // 设置输出参数
    uint8_t* dstdata[1] = { mat_frame.data };
    int dstlinesize[1] = { (int)mat_frame.step };

    // 执行转换,获得cv::Mat
    sws_scale(
        sws_ctx,
        frame->data,
        frame->linesize,
        0,
        frame->height,
        dstdata,
        dstlinesize
    );
    sws_freeContext(sws_ctx);

    cv::Mat resized_frame;
    cv::resize(mat_frame, resized_frame, cv::Size(input_resize, input_resize));
    // 转换通道
    cv::cvtColor(resized_frame, resized_frame, cv::COLOR_BGR2RGB); 
    // 归一化等操作
    std::vector<float> input_data = preprocess(resized_frame); 

    resized_frame.convertTo(resized_frame, CV_32FC3);
    resized_frame /= 255.0f; // 归一化到 [0,1]

    // --- 2. 转换为 CHW 格式 
    cv::Mat out;
    cv::dnn::blobFromImage(
        float_image,
        out,                // 输出 blob
        1.0,                     // 缩放因子
        cv::Size(),               // 不调整尺寸
        cv::Scalar(),             // 不减去均值
        false,                    // 不交换 BGR 通道
        false,                    // 不裁剪
        CV_32F                    // 输出数据类型
    );
    input_data.assign(
        out.ptr<float>(),
        out.ptr<float>() + chw_blob.total()
    );

}

void Inference::Startdemuxdecode()
{



}

Result Inference::StartInference(int64_t pts)
{
    if (!input_data.data() || !input_shape.data())
    {
        LOGERROR("input error!");
        return Result();
    }
    // 4. 创建输入张量
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(
        OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault
    );
    std::vector<Ort::Value> input_tensors;
    input_tensors.emplace_back(Ort::Value::CreateTensor<float>(
        memory_info,
        input_data.data(),
        input_data.size(),
        input_shape.data(),
        input_shape.size()
    ));

    const char* input_names[] = { "input" };
    const char* output_names[] = { "output" };
    auto output_tensors = session.Run(
        Ort::RunOptions{ nullptr },
        input_names,
        input_tensors.data(),
        1,
        output_names,
        1
    );
    float* output_data = output_tensors[0].GetTensorMutableData<float>();
    float auc_score = output_data[0]; 
    float f1_score = output_data[1];
    Result res;
    res.AUC = auc_score;
    res.F1 = f1_score;
    res.pts = pts;
    return res;
}
