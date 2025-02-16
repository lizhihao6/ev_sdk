//
// Created by hrh on 2019-09-02.
//

#ifndef JI_SAMPLEDETECTOR_HPP
#define JI_SAMPLEDETECTOR_HPP
#include <string>
#include <opencv2/core/mat.hpp>
#include "yolox.hpp"
#include "ji_utils.h"
extern "C"
{
#include "list.h"
}
#define STATUS int
using namespace std;

/**
 * 使用darknet实现的目标分类器，分类类别使用imagenet1k数据
 */

class SampleDetector
{
public:
    typedef struct
    {
        float prob;
        string name;
        cv::Rect rect;
    } Object;

    typedef struct
    {
        double nms;
        double thresh;
        double hierThresh;
    } ALGO_CONFIG_TYPE;

    SampleDetector(double thresh, double nms, double hierThresh);
    STATUS init();
    void unInit();
    STATUS processImage(const cv::Mat &image, vector<Object> &detectResults);

    bool setThresh(double thresh);

public:
    static const int ERROR_BASE = 0x0100;
    static const int ERROR_INVALID_INPUT = 0x0101;
    static const int ERROR_INVALID_INIT_ARGS = 0x0102;
    static const int ERROR_INVALID_YOLOX_PATH = 0x0103;

    static const int PROCESS_OK = 0x1001;
    static const int INIT_OK = 0x1002;

private:
    // network* mNetworkPtr{nullptr};
    YOLOX *yolox = nullptr;
    const string yoloxPath = "/usr/local/ev_sdk/model/model_trt.engine";
    char *classNamePath = "/usr/local/ev_sdk/config/coco.names";

    char **mLabels = nullptr;
    double mThresh = 0.5;
    double mHIERThresh = 0.5;
    double mNms = 0.6;
    size_t mClasses = 0;
};

#endif //JI_SAMPLEDETECTOR_HPP
