#ifndef JI_YOLOX_HPP
#define JI_YOLOX_HPP

#include <fstream>
#include <iostream>
#include <sstream>
#include <numeric>
#include <chrono>
#include <vector>
#include <opencv2/opencv.hpp>
#include <dirent.h>
#include "NvInfer.h"
#include "cuda_runtime_api.h"
#include "logging.h"

#define CHECK(status) \
    do\
    {\
        auto ret = (status);\
        if (ret != 0)\
        {\
            std::cerr << "Cuda failure: " << ret << std::endl;\
            abort();\
        }\
    } while (0)

#define DEVICE 0  // GPU id
#define NMS_THRESH 0.45
#define BBOX_CONF_THRESH 0.3
#define INPUT_W  640
#define INPUT_H  640
#define NUM_CLASSES 5

using namespace nvinfer1;

typedef struct {
    cv::Rect_<float> rect;
    int label;
    float prob;
} YOLOXObject;
typedef struct{
    int grid0;
    int grid1;
    int stride;
} GridAndStride;

static Logger gLogger;

static void generate_grids_and_stride(const int target_size, std::vector<int>& strides, std::vector<GridAndStride>& grid_strides);
static inline float intersection_area(const YOLOXObject& a, const YOLOXObject& b);
static void qsort_descent_inplace(std::vector<YOLOXObject>& faceobjects, int left, int right);
static void qsort_descent_inplace(std::vector<YOLOXObject>& objects);
static void nms_sorted_bboxes(const std::vector<YOLOXObject>& faceobjects, std::vector<int>& picked, float nms_threshold);
static void generate_yolox_proposals(std::vector<GridAndStride> grid_strides, float* feat_blob, float prob_threshold, std::vector<YOLOXObject>& objects);
static void decode_outputs(float* prob, std::vector<YOLOXObject>& objects, float scale, const int img_w, const int img_h);

class YOLOX {
    public:
        YOLOX();
        int init(const std::string engine_file_path);
        std::vector<YOLOXObject>  forward(const cv::Mat &img);
        void uninit();

    private:
        const char* INPUT_BLOB_NAME = "input_0";
        const char* OUTPUT_BLOB_NAME = "output_0";
        int output_size = 1;
        float *prob = nullptr;

        IRuntime* runtime = nullptr;
        ICudaEngine* engine = nullptr;
        IExecutionContext* context = nullptr;
        
        cv::Mat static_resize(const cv::Mat& img) ;
       
        float* blobFromImage(cv::Mat& img);
        void doInference(IExecutionContext& context, float* input, float* output, const int output_size, cv::Size input_shape);
};


#endif //JI_YOLOX_HPP