#ifndef YOLOV5_UTILS_HPP
#define YOLOV5_UTILS_HPP
#include <cmath>
#include <algorithm>
#include <string.h>
#include <iostream>
#include "NvInfer.h"
#include "logging.h"
extern "C"
{
#include "list.h"
}

using namespace std;
using namespace nvinfer1;

static Logger gLogger;
static int get_width(int x, float gw);
static int get_depth(int x, float gd);
int init_labels(char **mLabels, size_t &mClasses, const string classNamePath);
int get_gd_gw(float &gd, float &gw);
int wts_to_engine(const string wts_file_path, string engine_file_path);
ICudaEngine *build_engine(unsigned int maxBatchSize, IBuilder *builder, IBuilderConfig *config, DataType dt, float &gd, float &gw, const string &wts_name);
void APIToModel(unsigned int maxBatchSize, IHostMemory **modelStream, float &gd, float &gw, const string &wts_name);
bool replace(string &str, const string &from, const string &to);
#endif // YOLOV5_UTILS_HPP