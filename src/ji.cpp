/**
 * 示例代码：实现ji.h定义的图像接口，开发者需要根据自己的实际需求对接口进行实现
 */

#include <cstdlib>
#include <cstring>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <glog/logging.h>

#include "encrypt_wrapper.hpp"
#include "ji_license.h"
#include "ji_license_impl.h"
#include "WKTParser.h"
#include "cJSON.h"
#include <ji_utils.h>
#include "ji.h"

#include "pubKey.hpp"
#include "model_str.hpp"
#include "SampleDetector.hpp"
#include "Configuration.hpp"

#define JSON_ALERT_FLAG_KEY ("alert_flag")
#define JSON_ALERT_FLAG_TRUE 1
#define JSON_ALERT_FLAG_FALSE 0

// 如果需要添加授权功能，请保留该宏定义，并在ji_init中实现授权校验
#define ENABLE_JI_AUTHORIZATION
// 如果需要加密模型，请保留该宏定义，并在ji_create_predictor中实现模型解密
// #define ENABLE_JI_MODEL_ENCRYPTION

#ifndef EV_SDK_DEBUG
#define EV_SDK_DEBUG 1
#endif

cv::Mat outputFrame{0};     // 用于存储算法处理后的输出图像，根据ji.h的接口规范，接口实现需要负责释放该资源
char *jsonResult = nullptr; // 用于存储算法处理后输出到JI_EVENT的json字符串，根据ji.h的接口规范，接口实现需要负责释放该资源
Configuration config;

/**
 * @brief 使用predictor对输入图像inFrame进行处理
 *
 * @param[in] predictor 算法句柄
 * @param[in] inFrame 输入图像
 * @param[in] args 处理当前输入图像所需要的输入参数，例如在目标检测中，通常需要输入ROI，由开发者自行定义和解析
 * @param[out] outFrame 输入图像，由内部填充结果，外部代码需要负责释放其内存空间
 * @param[out] event 以JI_EVENT封装的处理结果
 * @return 如果处理成功，返回JISDK_RET_SUCCEED
 */
int processMat(SampleDetector *detector, const cv::Mat &inFrame, const char *args, cv::Mat &outFrame, JI_EVENT &event)
{
    // 处理输入图像
    if (inFrame.empty())
    {
        return JISDK_RET_FAILED;
    }

#ifdef ENABLE_JI_AUTHORIZATION
    // 检查授权，统计QPS
    int ret = ji_check_expire();
    if (ret != JISDK_RET_SUCCEED)
    {
        switch (ret)
        {
        case EV_OVERMAXQPS:
            return JISDK_RET_OVERMAXQPS;
            break;
        case EV_OFFLINE:
            return JISDK_RET_OFFLINE;
            break;
        default:
            return JISDK_RET_UNAUTHORIZED;
        }
    }
#endif
    if (inFrame.cols != config.currentInFrameSize.width || inFrame.rows != config.currentInFrameSize.height)
    {
        config.updateROIInfo(inFrame.cols, inFrame.rows);
    }

    /**
     * 解析参数并更新，根据接口规范标准，接口必须支持配置文件/usr/local/ev_sdk/model/algo_config.json内参数的实时更新功能
     * （即通过ji_calc_*等接口传入）
     */
    ALGO_CONFIG_TYPE algoConfig = config.parseAndUpdateArgs(args);
    detector->setThresh(algoConfig.thresh);

    // 针对每个ROI进行算法处理
    std::vector<SampleDetector::Object> detectedObjects;
    std::vector<SampleDetector::Object> validTargets;

    // 算法处理
    int processRet = detector->processImage(inFrame, detectedObjects);
    if (processRet != SampleDetector::PROCESS_OK)
    {
        return JISDK_RET_FAILED;
    }
    for (auto &obj : detectedObjects)
    {
        for (auto &roiPolygon : config.currentROIOrigPolygons)
        {
            int mid_x = obj.rect.x + obj.rect.width / 2;
            int mid_y = obj.rect.y + obj.rect.height / 2;
            // 当检测的目标在ROI内的话，就视为有效目标
            if (WKTParser::inPolygon(roiPolygon, cv::Point(mid_x, mid_y)))
            {
                validTargets.emplace_back(obj);
                break;
            }
        }
    }

    // 此处示例业务逻辑：当算法检测到有`dog`时，就报警
    bool isNeedAlert = false; // 是否需要报警

    // 创建输出图
    inFrame.copyTo(outFrame);
    // 画ROI区域
    if (config.drawROIArea && !config.currentROIOrigPolygons.empty())
    {
        drawPolygon(outFrame, config.currentROIOrigPolygons, cv::Scalar(config.roiColor[0], config.roiColor[1], config.roiColor[2]),
                    config.roiColor[3], cv::LINE_AA, config.roiLineThickness, config.roiFill);
    }
    // 判断是否要要报警并将检测到的目标画到输出图上
    if (validTargets.size() > 0)
    {
        isNeedAlert = true;
    }
    for (auto &object : validTargets)
    {
        LOG(INFO) << "Found " << object.name;
        if (config.drawResult)
        {
            std::stringstream ss;
            auto objName = config.targetRectTextMap_0[config.language];
            if (object.name == "grey_brick")
                objName = config.targetRectTextMap_1[config.language];
            else if (object.name == "bricks")
                objName = config.targetRectTextMap_2[config.language];
            else if (object.name == "mound")
                objName = config.targetRectTextMap_3[config.language];
            else
                objName = config.targetRectTextMap_4[config.language];

            ss << objName;
            if (config.drawConfidence)
            {
                ss.precision(2);
                ss << std::fixed << (objName.empty() ? "" : ": ") << object.prob * 100 << "%";
            }
            drawRectAndText(outFrame, object.rect, ss.str(), config.targetRectLineThickness, cv::LINE_AA,
                            cv::Scalar(config.targetRectColor[0], config.targetRectColor[1], config.targetRectColor[2]), config.targetRectColor[3], config.targetTextHeight,
                            cv::Scalar(config.textFgColor[0], config.textFgColor[1], config.textFgColor[2]),
                            cv::Scalar(config.textBgColor[0], config.textBgColor[1], config.textBgColor[2]));
        }
    }

    if (isNeedAlert && config.drawWarningText)
    {
        drawText(outFrame, config.warningTextMap[config.language], config.warningTextSize,
                 cv::Scalar(config.warningTextFg[0], config.warningTextFg[1], config.warningTextFg[2]),
                 cv::Scalar(config.warningTextBg[0], config.warningTextBg[1], config.warningTextBg[2]), config.warningTextLeftTop);
    }

    // 将结果封装成json字符串
    cJSON *rootObj = cJSON_CreateObject();
    cJSON *algorithm_data = cJSON_CreateObject();
    cJSON *model_data = cJSON_CreateObject();
    cJSON_AddItemToObject(rootObj, "algorithm_data", algorithm_data);
    cJSON_AddItemToObject(rootObj, "model_data", model_data);

    cJSON_AddItemToObject(algorithm_data, "is_alert", cJSON_CreateBool(isNeedAlert));
    cJSON_AddItemToObject(algorithm_data, "target_count", cJSON_CreateNumber(validTargets.size()));
    cJSON *target_info = cJSON_CreateArray();
    cJSON_AddItemToObject(algorithm_data, "target_info", target_info);

    cJSON *objects = cJSON_CreateArray();
    cJSON_AddItemToObject(model_data, "objects", objects);

    for (auto &object : detectedObjects)
    {
        cJSON *odbObj = cJSON_CreateObject();
        cJSON_AddItemToObject(odbObj, "x", cJSON_CreateNumber(object.rect.x));
        cJSON_AddItemToObject(odbObj, "y", cJSON_CreateNumber(object.rect.y));
        cJSON_AddItemToObject(odbObj, "height", cJSON_CreateNumber(object.rect.height));
        cJSON_AddItemToObject(odbObj, "width", cJSON_CreateNumber(object.rect.width));
        cJSON_AddItemToObject(odbObj, "confidence", cJSON_CreateNumber(object.prob));
        cJSON_AddItemToObject(odbObj, "name", cJSON_CreateString(object.name.c_str()));

        cJSON *_odbObj = cJSON_CreateObject();
        cJSON_AddItemToObject(_odbObj, "x", cJSON_CreateNumber(object.rect.x));
        cJSON_AddItemToObject(_odbObj, "y", cJSON_CreateNumber(object.rect.y));
        cJSON_AddItemToObject(_odbObj, "width", cJSON_CreateNumber(object.rect.width));
        cJSON_AddItemToObject(_odbObj, "height", cJSON_CreateNumber(object.rect.height));
        cJSON_AddItemToObject(_odbObj, "confidence", cJSON_CreateNumber(object.prob));
        cJSON_AddItemToObject(_odbObj, "name", cJSON_CreateString(object.name.c_str()));

        cJSON_AddItemToArray(objects, odbObj);
        cJSON_AddItemToArray(target_info, _odbObj);
    }

    char *jsonResultStr = cJSON_Print(rootObj);
    int jsonSize = strlen(jsonResultStr);
    if (jsonResult == nullptr)
    {
        jsonResult = new char[jsonSize + 1];
    }
    else if (strlen(jsonResult) < jsonSize)
    {
        free(jsonResult); // 如果需要重新分配空间，需要释放资源
        jsonResult = new char[jsonSize + 1];
    }
    strcpy(jsonResult, jsonResultStr);

    // 注意：JI_EVENT.code需要根据需要填充，切勿弄反
    if (isNeedAlert)
    {
        event.code = JISDK_CODE_ALARM;
    }
    else
    {
        event.code = JISDK_CODE_NORMAL;
    }
    event.json = jsonResult;

    if (rootObj)
        cJSON_Delete(rootObj);
    if (jsonResultStr)
        free(jsonResultStr);

    return JISDK_RET_SUCCEED;
}

void *ji_create_predictor(int pdtype)
{
#ifdef ENABLE_JI_AUTHORIZATION
    if (ji_check_expire_only() != EV_SUCCESS)
    {
        return nullptr;
    }
#endif

    // 从统一的配置文件读取配置参数，SDK实现必须支持从这个统一的配置文件中读取算法&业务逻辑相关的配置参数
    const char *configFile = "/usr/local/ev_sdk/config/algo_config.json";
    LOG(INFO) << "Parsing configuration file: " << configFile;

    ALGO_CONFIG_TYPE algoConfig{config.mAlgoConfigDefault.nms, config.mAlgoConfigDefault.thresh, config.mAlgoConfigDefault.hierThresh};
    std::ifstream confIfs(configFile);
    if (confIfs.is_open())
    {
        size_t len = getFileLen(confIfs);
        char *confStr = new char[len + 1];
        confIfs.read(confStr, len);
        confStr[len] = '\0';

        algoConfig = config.parseAndUpdateArgs(confStr);
        config.mAlgoConfigDefault = algoConfig;
        delete[] confStr;
        confIfs.close();
    }

    auto *detector = new SampleDetector(algoConfig.thresh, algoConfig.nms, algoConfig.hierThresh);
    char *decryptedModelStr = nullptr;

#ifdef ENABLE_JI_MODEL_ENCRYPTION
    LOG(INFO) << "Decrypting model...";
    // 如果使用了模型加密功能，需要将加密后的模型（放在`model_str.hpp`内）进行解密
    void *h = CreateDecryptor(model_str.c_str(), model_str.size(), key.c_str());

    // 获取解密后的字符串
    int fileLen = 0;
    decryptedModelStr = (char *)FetchBuffer(h, fileLen);
    char *tmp = new char[fileLen + 1];
    strncpy(tmp, decryptedModelStr, fileLen);
    tmp[fileLen] = '\0';
    decryptedModelStr = tmp;
    LOG(INFO) << "Decrypted model size:" << strlen(decryptedModelStr);

    // 如何想要使用解密后的文件句柄，请调用这个接口
    // FILE *file = (file *) FetchFile(h);

    DestroyDecrtptor(h);
#else
    // 不使用模型加密功能，直接从模型文件读取
    std::ifstream ifs = std::ifstream("/usr/local/ev_sdk/model/yolov3-tiny.cfg", std::ios::binary);
    long len = getFileLen(ifs);
    decryptedModelStr = new char[len + 1];
    ifs.read(decryptedModelStr, len);
    decryptedModelStr[len] = '\0';
#endif

    int iRet = detector->init();
    if (decryptedModelStr != nullptr)
    {
        free(decryptedModelStr);
    }
    if (iRet != SampleDetector::INIT_OK)
    {
        return nullptr;
    }
    LOG(INFO) << "SamplePredictor init OK.";

    return detector;
}

void ji_destroy_predictor(void *predictor)
{
    if (predictor == NULL)
        return;

    auto *detector = reinterpret_cast<SampleDetector *>(predictor);
    detector->unInit();
    delete detector;
}

int ji_init(int argc, char **argv)
{
    LOG(INFO) << "EV_SDK version:" << EV_SDK_VERSION;
    int authCode = JISDK_RET_SUCCEED;
#ifdef ENABLE_JI_AUTHORIZATION
    // Get license version
    char *license_version = nullptr;
    ji_get_license_version(&license_version);
    LOG(INFO) << "License version:" << license_version;
    free(license_version);

    // 检查license参数
    if (argc < 6)
    {
        return JISDK_RET_INVALIDPARAMS;
    }

    if (argv[0] == NULL || argv[5] == NULL)
    {
        return JISDK_RET_INVALIDPARAMS;
    }

    int qps = 0;
    if (argv[4])
        qps = atoi(argv[4]);

    // 使用公钥校验授权信息
    int ret = ji_check_license(pubKey, argv[0], argv[1], argv[2], argv[3], qps > 0 ? &qps : NULL, atoi(argv[5]));
    if (ret != EV_SUCCESS)
    {
        authCode = JISDK_RET_UNAUTHORIZED;
    }
#endif
    if (authCode != JISDK_RET_SUCCEED)
    {
        LOG(ERROR) << "ji_check_license failed!";
        return authCode;
    }

    return authCode;
}

void ji_reinit()
{
#ifdef ENABLE_JI_AUTHORIZATION
    ji_check_license(NULL, NULL, NULL, NULL, NULL, NULL, 0);
#endif
    if (jsonResult)
    {
        free(jsonResult);
        jsonResult = nullptr;
    }
}

int ji_calc_frame(void *predictor, const JI_CV_FRAME *inFrame, const char *args,
                  JI_CV_FRAME *outFrame, JI_EVENT *event)
{
    if (predictor == NULL || inFrame == NULL)
    {
        return JISDK_RET_INVALIDPARAMS;
    }

    auto *detector = reinterpret_cast<SampleDetector *>(predictor);
    cv::Mat inMat(inFrame->rows, inFrame->cols, inFrame->type, inFrame->data, inFrame->step);
    if (inMat.empty())
    {
        return JISDK_RET_FAILED;
    }
    int processRet = processMat(detector, inMat, args, outputFrame, *event);

    if (processRet == JISDK_RET_SUCCEED)
    {
        if ((event->code != JISDK_CODE_FAILED) && (!outputFrame.empty()) && (outFrame))
        {
            outFrame->rows = outputFrame.rows;
            outFrame->cols = outputFrame.cols;
            outFrame->type = outputFrame.type();
            outFrame->data = outputFrame.data;
            outFrame->step = outputFrame.step;
        }
    }
    return processRet;
}

int ji_calc_buffer(void *predictor, const void *buffer, int length, const char *args, const char *outFile,
                   JI_EVENT *event)
{
    if (predictor == NULL || buffer == NULL || length <= 0)
    {
        return JISDK_RET_INVALIDPARAMS;
    }

    auto *classifierPtr = reinterpret_cast<SampleDetector *>(predictor);

    const unsigned char *b = (const unsigned char *)buffer;
    std::vector<unsigned char> vecBuffer(b, b + length);
    cv::Mat inMat = cv::imdecode(vecBuffer, cv::IMREAD_COLOR);
    if (inMat.empty())
    {
        return JISDK_RET_FAILED;
    }

    cv::Mat outMat;
    int processRet = processMat(classifierPtr, inMat, args, outMat, *event);

    if (processRet == JISDK_RET_SUCCEED)
    {
        if ((event->code != JISDK_CODE_FAILED) && (!outMat.empty()) && (outFile))
        {
            cv::imwrite(outFile, outMat);
        }
    }
    return processRet;
}

int ji_calc_file(void *predictor, const char *inFile, const char *args, const char *outFile, JI_EVENT *event)
{
    if (predictor == NULL || inFile == NULL)
    {
        return JISDK_RET_INVALIDPARAMS;
    }

    auto *classifierPtr = reinterpret_cast<SampleDetector *>(predictor);
    cv::Mat inMat = cv::imread(inFile);
    if (inMat.empty())
    {
        return JISDK_RET_FAILED;
    }

    cv::Mat outMat;
    int processRet = processMat(classifierPtr, inMat, args, outMat, *event);
    if (processRet == JISDK_RET_SUCCEED)
    {
        if ((event->code != JISDK_CODE_FAILED) && (!outMat.empty()) && (outFile))
        {
            cv::imwrite(outFile, outMat);
        }
    }

    return processRet;
}

int ji_calc_video_file(void *predictor, const char *infile, const char *args,
                       const char *outfile, const char *jsonfile)
{
    // 没有实现的接口必须返回`JISDK_RET_UNUSED`
    if (predictor == NULL || infile == NULL)
    {
        return JISDK_RET_INVALIDPARAMS;
    }
    auto *classifierPtr = reinterpret_cast<SampleDetector *>(predictor);

    cv::VideoCapture videoCapture(infile);
    if (!videoCapture.isOpened())
    {
        return JISDK_RET_FAILED;
    }

    cv::VideoWriter vwriter;
    cv::Mat inMat, outMat;
    JI_EVENT event;
    int iRet = JISDK_RET_FAILED;
    int totalFrames, alertFrames, timestamp;
    totalFrames = alertFrames = timestamp = 0;

    cJSON *jsonRoot, *jsonDetail;
    jsonRoot = jsonDetail = NULL;

    while (videoCapture.read(inMat))
    {
        timestamp = videoCapture.get(cv::CAP_PROP_POS_MSEC);

        iRet = processMat(classifierPtr, inMat, args, outMat, event);

        if (iRet == JISDK_RET_SUCCEED)
        {
            ++totalFrames;

            if (event.code != JISDK_CODE_FAILED)
            {
                if (event.code == JISDK_CODE_ALARM)
                {
                    ++alertFrames;
                }

                if (!outMat.empty() && outfile)
                {
                    if (!vwriter.isOpened())
                    {
                        vwriter.open(outfile,
                                     /*videoCapture.get(cv::CAP_PROP_FOURCC)*/ cv::VideoWriter::fourcc('X', '2', '6', '4'),
                                     videoCapture.get(cv::CAP_PROP_FPS), outMat.size());
                        if (!vwriter.isOpened())
                        {
                            return JISDK_RET_FAILED;
                        }
                    }
                    vwriter.write(outMat);
                }

                if (event.json && jsonfile)
                {
                    if (jsonDetail == NULL)
                    {
                        jsonDetail = cJSON_CreateArray();
                    }

                    cJSON *jsonFrame = cJSON_Parse(event.json);
                    if (jsonFrame)
                    {
                        cJSON_AddItemToObjectCS(jsonFrame, "timestamp", cJSON_CreateNumber(timestamp));
                        cJSON_AddItemToArray(jsonDetail, jsonFrame);
                    }
                }
            }
        }
        else
        {
            break;
        }
    }

    if (iRet == JISDK_RET_SUCCEED)
    {
        if (jsonfile)
        {
            jsonRoot = cJSON_CreateObject();
            cJSON_AddItemToObjectCS(jsonRoot, "total_frames", cJSON_CreateNumber(totalFrames));
            cJSON_AddItemToObjectCS(jsonRoot, "alert_frames", cJSON_CreateNumber(alertFrames));

            if (jsonDetail)
            {
                cJSON_AddItemToObjectCS(jsonRoot, "detail", jsonDetail);
            }

            char *buff = cJSON_Print(jsonRoot);
            std::ofstream fs(jsonfile);
            if (fs.is_open())
            {
                fs << buff;
                fs.close();
            }
            free(buff);
        }
    }

    if (jsonRoot)
    {
        cJSON_Delete(jsonRoot);
    }
    else if (jsonDetail)
    {
        cJSON_Delete(jsonDetail);
    }

    return iRet;
}