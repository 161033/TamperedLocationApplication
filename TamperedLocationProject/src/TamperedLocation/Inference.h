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
	//��ʼ��ONNX Runtime����
	void Init();

	//Ԥ������Ƶ֡��������������
	void Preprocess(AVFrame* frame);

	void Startdemuxdecode();

	Result StartInference(int64_t pts);

private:
	std::vector<int64_t> input_shape;
	std::vector<float> input_data;
	//std::unique_ptr<Ort::Session> session;
};