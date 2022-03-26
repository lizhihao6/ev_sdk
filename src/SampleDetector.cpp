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
    mLabels = get_labels(classNamePath);
    if (mLabels == nullptr)
    {
        LOG(ERROR) << "Failed getting labels from `" << classNamePath << "`!";
        return SampleDetector::ERROR_INVALID_INIT_ARGS;
    }
    while (mLabels[mClasses] != nullptr)
    {
        mClasses++;
    }
    cout << "Num of Classes " << mClasses << endl;

    yolox = new YOLOX();
    // if (yolox->init(yoloxPath))
    // {
    //     LOG(ERROR) << "Failed loading weights from `" << yoloxPath << "`!";
    //     return SampleDetector::ERROR_INVALID_YOLOX_PATH;
    // }

    LOG(INFO) << "Done.";
    return SampleDetector::INIT_OK;
}

void SampleDetector::unInit()
{
    if (mLabels)
    {
        for (int i = 0; i < mClasses; ++i)
        {
            if (mLabels[i])
            {
                free(mLabels[i]);
                mLabels[i] = nullptr;
            }
        }
        free(mLabels);
        mLabels = nullptr;
    }

    // if (yolox)
    // {
    //     yolox->uninit();
    //     yolox = nullptr;
    // }
}

STATUS SampleDetector::processImage(const cv::Mat &cv_image, vector<Object> &result)
{
    if (cv_image.empty())
    {
        LOG(ERROR) << "Invalid input!";
        return ERROR_INVALID_INPUT;
    }
    vector<YOLOXObject> yoloxObject = yolox->forward(cv_image);
    for (auto object = yoloxObject.begin(); object != yoloxObject.end(); object++)
    {
        if (object->prob > mThresh)
            result.push_back(Object{object->prob, mLabels[object->label], object->rect});
    }

    // todo: maybe need del yoloxObject
    return SampleDetector::PROCESS_OK;
}

STATUS SampleDetector::processDiff(const cv::Mat &ref_image, const cv::Mat &cv_image, vector<Object> &result, std::vector<std::vector<cv::Point>> &roi)
{
    if (ref_image.empty() || cv_image.empty())
    {
        LOG(ERROR) << "Invalid input!";
        return ERROR_INVALID_INPUT;
    }
    
    // convert to gray mask
    cv::Mat ref_gray, gray, diffImage, mask;
    cv::cvtColor(ref_image, ref_gray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(cv_image, gray, cv::COLOR_BGR2GRAY);

    if(roi.size()>0){
        cv::Mat roi_mask = cv::Mat::zeros(gray.size(), CV_8UC1); 
        cv::fillPoly(roi_mask, roi, cv::Scalar(255));
        gray.copyTo(gray, roi_mask);
        ref_gray.copyTo(ref_gray, roi_mask);
    }

    cv::GaussianBlur(ref_gray, ref_gray, cv::Size(gaussianBlurSize, gaussianBlurSize), 0);
    cv::GaussianBlur(gray, gray, cv::Size(gaussianBlurSize, gaussianBlurSize), 0);
    
    cv::absdiff(ref_gray, gray, diffImage);

    // const float diffThresh = (float)mThresh * maxDiff; // mapping the prob from 0 to 25
    threshold(diffImage, mask, minDiff, 255, cv::THRESH_BINARY);
    
    dilate(mask, mask, cv::Mat(), cv::Point(-1, -1), 2);
    vector<vector<cv::Point>> cnts;
    cv::findContours(mask, cnts, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (auto cnt = cnts.begin(); cnt != cnts.end(); cnt++)
    {
        cv::Rect rect = cv::boundingRect(*cnt);
        if (rect.width < 30 || rect.height < 30)
            continue;
        cv::Mat roi_diff = diffImage(rect);
        cv::Scalar mean = cv::mean(roi_diff);
        float prob = (float)mean[0] / maxDiff;
        prob = std::min(prob, 1.0f);
        if (prob > mThresh)
            result.push_back(Object{prob, "garbage", rect});
    }
    return SampleDetector::PROCESS_OK;
}

bool SampleDetector::setThresh(double thresh)
{
    mThresh = thresh;
    return true;
}
