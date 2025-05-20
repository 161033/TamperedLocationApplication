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
    // ��ʼ�� ONNX Runtime ����
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "VideoAuthenticityDetector");
    Ort::SessionOptions session_options;
    session = Ort::Session::Create(env, model, session_options);

    // ��ȡ���������Ϣ
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::TypeInfo input_type_info = session.GetInputTypeInfo(0);
    auto input_tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
    input_shape = input_tensor_info.GetShape();

}

void Inference::Preprocess(AVFrame* frame)
{
    // ���������Ч��
    if (!frame || !frame->data[0]) return;

    // ����ת�������� (YUV420P -> BGR24)
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
    // �����������
    uint8_t* dstdata[1] = { mat_frame.data };
    int dstlinesize[1] = { (int)mat_frame.step };

    // ִ��ת��,���cv::Mat
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
    // ת��ͨ��
    cv::cvtColor(resized_frame, resized_frame, cv::COLOR_BGR2RGB); 
    // ��һ���Ȳ���
    std::vector<float> input_data = preprocess(resized_frame); 

    resized_frame.convertTo(resized_frame, CV_32FC3);
    resized_frame /= 255.0f; // ��һ���� [0,1]

    // --- 2. ת��Ϊ CHW ��ʽ 
    cv::Mat out;
    cv::dnn::blobFromImage(
        float_image,
        out,                // ��� blob
        1.0,                     // ��������
        cv::Size(),               // �������ߴ�
        cv::Scalar(),             // ����ȥ��ֵ
        false,                    // ������ BGR ͨ��
        false,                    // ���ü�
        CV_32F                    // �����������
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
    // 4. ������������
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
