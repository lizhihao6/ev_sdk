#include "yolov5.hpp"

YOLOV5::YOLOV5(const float threshold) : mThreshold(threshold)
{
}

int YOLOV5::init(const string wts_file_path, const string classNamePath)
{
    if (init_labels(mLabels, mClasses, classNamePath))
        return -1;

    cudaSetDevice(DEVICE);
    string engine_file_path = wts_file_path;
    replace(engine_file_path, "wts", "engine");
    wts_to_engine(wts_file_path, engine_file_path);
    loadEngine(engine_file_path);
    assert(runtime != nullptr);
    assert(engine != nullptr);
    assert(context != nullptr);

    // create GPU buffer
    assert(engine->getNbBindings() == 2);
    CUDA_CHECK(cudaMalloc(&buffers[inputIndex], BATCH_SIZE * 3 * INPUT_H * INPUT_W * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&buffers[outputIndex], BATCH_SIZE * OUTPUT_SIZE * sizeof(float)));

    // create stream
    CUDA_CHECK(cudaStreamCreate(&stream));

    // init data & prob
    data = new float[BATCH_SIZE * 3 * INPUT_H * INPUT_W];
    prob = new float[BATCH_SIZE * 3 * OUTPUT_SIZE];

    return 0;
}

void YOLOV5::forward(const cv::Mat &img, vector<Object> &results)
{
    cv::Mat pr_img = preprocess_img(img, INPUT_W, INPUT_H); // letterbox BGR to RGB
    // todo edit here
    int i = 0;
    for (int row = 0; row < INPUT_H; ++row)
    {
        uchar *uc_pixel = pr_img.data + row * pr_img.step;
        for (int col = 0; col < INPUT_W; ++col)
        {
            data[i] = (float)uc_pixel[2] / 255.0;
            data[i + INPUT_H * INPUT_W] = (float)uc_pixel[1] / 255.0;
            data[i + 2 * INPUT_H * INPUT_W] = (float)uc_pixel[0] / 255.0;
            uc_pixel += 3;
            ++i;
        }
    }

    // Run inference
    auto start = chrono::system_clock::now();
    doInference(data, prob);
    auto end = chrono::system_clock::now();
    cout << chrono::duration_cast<chrono::milliseconds>(end - start).count() << "ms" << endl;

    // append results
    vector<Yolo::Detection> res;
    nms(res, &prob[0 * OUTPUT_SIZE], mThreshold, NMS_THRESH);
    for (auto r : res)
    {
        results.push_back({r.conf, mLabels[(int)r.class_id], get_rect(img, r.bbox)});
    }
}

void YOLOV5::uninit()
{
    // Remove labels
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
    // Release stream and buffers
    cudaStreamDestroy(stream);
    CUDA_CHECK(cudaFree(buffers[inputIndex]));
    CUDA_CHECK(cudaFree(buffers[outputIndex]));
    // Destroy the engine
    context->destroy();
    engine->destroy();
    runtime->destroy();

    // delete data & prob
    delete[] data;
    delete[] prob;
}

void YOLOV5::doInference(float *input, float *output)
{
    // DMA input batch data to device, infer on the batch asynchronously, and DMA output back to host
    CUDA_CHECK(cudaMemcpyAsync(buffers[0], input, BATCH_SIZE * 3 * INPUT_H * INPUT_W * sizeof(float), cudaMemcpyHostToDevice, stream));
    context->enqueue(BATCH_SIZE, buffers, stream, nullptr);
    CUDA_CHECK(cudaMemcpyAsync(output, buffers[1], BATCH_SIZE * OUTPUT_SIZE * sizeof(float), cudaMemcpyDeviceToHost, stream));
    cudaStreamSynchronize(stream);
}

void YOLOV5::loadEngine(string engine_file_path)
{
    ifstream file(engine_file_path, ios::binary);
    size_t size = 0;
    file.seekg(0, file.end);
    size = file.tellg();
    file.seekg(0, file.beg);
    char *trtModelStream = new char[size];
    assert(trtModelStream);
    file.read(trtModelStream, size);
    file.close();
    runtime = createInferRuntime(gLogger);
    engine = runtime->deserializeCudaEngine(trtModelStream, size);
    context = engine->createExecutionContext();
    delete[] trtModelStream;
}