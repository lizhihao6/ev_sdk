#ifndef JI_YOLOV5_HPP
#define JI_YOLOV5_HPP

#include <iostream>
#include <chrono>
#include <cmath>
#include <string.h>
#include "cuda_utils.h"
#include "logging.h"
#include "utils.h"
#include "calibrator.h"
#include "object.hpp"
#include "NvInfer.h"
#include "yolov5_utils.hpp"
#include "common.hpp"

using namespace std;
using namespace Yolo;
using namespace nvinfer1;

void APIToModel(unsigned int maxBatchSize, IHostMemory **modelStream, float &gd, float &gw, const string &wts_name);
bool replace(string &str, const string &from, const string &to);

class YOLOV5
{
public:
    YOLOV5(const float threshold);
    int init(const string wts_file_path, const string classNamePath);
    void forward(const cv::Mat &img, vector<Object> &results);
    void uninit();

private:
    void doInference(float *input, float *output);
    void loadEngine(string engine_file_path);
    float mThreshold = 0.;
    char **mLabels = nullptr;
    size_t mClasses = 0;
    float *data = nullptr;
    float *prob = nullptr;
    IRuntime *runtime = nullptr;
    ICudaEngine *engine = nullptr;
    IExecutionContext *context = nullptr;
    const int inputIndex = 0;
    const int outputIndex = 1;
    void *buffers[2];
    cudaStream_t stream;
};

#endif //JI_YOLOV5_HPP