#ifndef JI_OBJECT_HPP
#define JI_OBJECT_HPP
#include <string>
#include <opencv2/core/mat.hpp>
typedef struct {
    float prob;
    string name;
    cv::Rect rect;
} Object;
#endif //JI_OBJECT_HPP