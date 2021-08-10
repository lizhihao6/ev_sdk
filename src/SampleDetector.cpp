//
// Created by hrh on 2019-09-02.
//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <glog/logging.h>
#include "SampleDetector.hpp"

SampleDetector::SampleDetector(double thresh, double nms, double hierThresh):
    mNms(nms), mThresh(thresh), mHIERThresh(hierThresh) {
    LOG(INFO) << "Current config: nms:" << mNms << ", thresh:" << mThresh
            << ", HIERThresh:" << mHIERThresh;
}

int SampleDetector::init() {
    LOG(INFO) << "Loading model...";
    mLabels = get_labels(classNamePath);
    if (mLabels == nullptr) {
        LOG(ERROR) << "Failed getting labels from `" << classNamePath << "`!";
        return SampleDetector::ERROR_INVALID_INIT_ARGS;
    }
    while (mLabels[mClasses] != nullptr) {
        mClasses++;
    }
    cout << "Num of Classes " << mClasses << endl;

    yolox = new YOLOX();
    if(yolox->init(yoloxPath)){
        LOG(ERROR) << "Failed loading weights from `" << yoloxPath << "`!";
        return SampleDetector::ERROR_INVALID_YOLOX_PATH;
    }

    LOG(INFO) << "Done.";
    return SampleDetector::INIT_OK;
}

void SampleDetector::unInit() {
    if (mLabels) {
        for (int i = 0; i < mClasses; ++i) {
            if (mLabels[i]) {
                free(mLabels[i]);
                mLabels[i] = nullptr;
            }
        }
        free(mLabels);
        mLabels = nullptr;
    }

    if(yolox){
        yolox->uninit();
        yolox = nullptr;
    }
}

STATUS SampleDetector::processImage(const cv::Mat &cv_image, vector<Object> &result) {
    if (cv_image.empty()) {
        LOG(ERROR) << "Invalid input!";
        return ERROR_INVALID_INPUT;
    }
    vector<YOLOXObject> yoloxObject = yolox->forward(cv_image);
    for(auto object = yoloxObject.begin(); object != yoloxObject.end(); object++){
        if(object->prob > mThresh)
            result.push_back(Object{object->prob, mLabels[object->label], object->rect});
    }

    // todo: maybe need del yoloxObject
    return SampleDetector::PROCESS_OK;
}

bool SampleDetector::setThresh(double thresh) {
    mThresh = thresh;
    return true;
}
