#pragma once
#include<vector>
//#include <onnxruntime_cxx_api.h>

class AVFrame;

struct Result
{
	float AUC;
	float F1;
	int64_t pts;
};

class Inference
{
public:
	Inference();
	//初始化ONNX Runtime环境
	void Init();

	//预处理视频帧并创建输入张量
	void Preprocess(AVFrame* frame);

	void Startdemuxdecode();

	Result StartInference(int64_t pts);

private:
	std::vector<int64_t> input_shape;
	std::vector<float> input_data;
	//std::unique_ptr<Ort::Session> session;
};