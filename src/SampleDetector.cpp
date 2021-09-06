//
// Created by hrh on 2019-09-02.
//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <glog/logging.h>
#include "SampleDetector.hpp"

SampleDetector::SampleDetector(double thresh, double nms, double hierThresh) : mNms(nms), mThresh(thresh), mHIERThresh(hierThresh)
{
    LOG(INFO) << "Current config: nms:" << mNms << ", thresh:" << mThresh
              << ", HIERThresh:" << mHIERThresh;
}

int SampleDetector::init()
{
    LOG(INFO) << "Loading model...";

    yolov5 = new YOLOV5(mThresh);
    if (yolov5->init(yolov5Path, classNamePath))
    {
        LOG(ERROR) << "Failed loading weights from `" << yolov5Path << "`!";
        return SampleDetector::ERROR_INVALID_YOLOX_PATH;
    }

    LOG(INFO) << "Done.";
    return SampleDetector::INIT_OK;
}

void SampleDetector::unInit()
{
    if (yolov5)
    {
        yolov5->uninit();
        yolov5 = nullptr;
    }
}

STATUS SampleDetector::processImage(const cv::Mat &cv_image, vector<Object> &result)
{
    if (cv_image.empty())
    {
        LOG(ERROR) << "Invalid input!";
        return ERROR_INVALID_INPUT;
    }
    yolov5->forward(cv_image, result);
    return SampleDetector::PROCESS_OK;
}

bool SampleDetector::setThresh(double thresh)
{
    mThresh = thresh;
    return true;
}
