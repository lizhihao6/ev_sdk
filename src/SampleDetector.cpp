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

STATUS SampleDetector::processDiff(const cv::Mat &ref_image, const cv::Mat &cv_image, vector<Object> &result)
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
    cv::absdiff(ref_gray, gray, diffImage);
    threshold(diffImage, mask, diffThresh, 255, cv::THRESH_BINARY);
    cv::Mat element = getStructuringElement(cv::MORPH_OPEN, cv::Size(kernelSize, kernelSize));
    morphologyEx(mask, mask, cv::MORPH_OPEN, element);
    
    // calculate the bounding box
    cv::Mat out, stats, centroids;
    int number = connectedComponentsWithStats(mask, out, stats,centroids, 8, CV_16U);
    for (int i = 1; i < number; i++){
        // int center_x = centroids.at<double>(i, 0);
        // int center_y = centroids.at<double>(i, 1);
        int x = stats.at<int>(i, cv::CC_STAT_LEFT);
        int y = stats.at<int>(i, cv::CC_STAT_TOP);
        int w = stats.at<int>(i, cv::CC_STAT_WIDTH);
        int h = stats.at<int>(i, cv::CC_STAT_HEIGHT);
        // int area = stats.at<int>(i, CC_STAT_AREA);
        // circle(img, Point(center_x, center_y), 2, Scalar(0, 255, 0), 2, 8, 0);
        cv::Rect rect(x, y, w, h);
        result.push_back(Object{0.9, "garbage", rect});
    }
    // todo: maybe need del yoloxObject
    return SampleDetector::PROCESS_OK;
}

bool SampleDetector::setThresh(double thresh)
{
    mThresh = thresh;
    return true;
}
