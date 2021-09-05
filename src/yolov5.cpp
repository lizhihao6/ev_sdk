#include "yolov5.hpp"
int YOLOV5::init(const std::string engine_file_path, const std::string classNamePath, const float threshold){
    // init mLabels
    mLabels = get_labels(classNamePath);
    if (mLabels == nullptr) {
        LOG(ERROR) << "Failed getting labels from `" << classNamePath << "`!";
        return -1;
    }
    while (mLabels[mClasses] != nullptr) {
        mClasses++;
    }
    cout << "Num of Classes " << mClasses << endl;

    mThreshold = threshold;

    cudaSetDevice(DEVICE);
    float gd = 0.0f, gw = 0.0f;
    switch(YOLO_MODEL_SIZE){
    case 's' :
        gd = 0.33;
        gw = 0.50;
        break;
    case 'm' :
        gd = 0.67;
        gw = 0.75;
        break;
    case 'l' :
        gd = 1.0;
        gw = 1.0;
        break;
    case 'x' :
        gd = 1.33;
        gw = 1.25;
        break;
    default :
        return -1;
   }
    string suffixStr = engine_file_path.substr(engine_file_path.find_last_of('.') + 1);
    if(suffixStr != "wts"){
        return -1;
    }
    engine_file_path.replace(engine_file_path.length()-3, 3, "engine");
    ifstream f(engine_file_path.c_str());
    if(!f.good()){
        IHostMemory* modelStream{ nullptr };
        APIToModel(BATCH_SIZE, &modelStream, false, gd, gw, engine_file_path);
        ofstream p(engine_file_path, ios::binary);
        if (!p) {
            std::cerr << "could not open plan output file" << std::endl;
            return -1;
        }
        modelStream->destroy();
        p.write(reinterpret_cast<const char*>(modelStream->data()), modelStream->size());
    }
    ifstream file(engine_file_path, ios::binary);
    size_t size = 0;
    file.seekg(0, file.end);
    size = file.tellg();
    file.seekg(0, file.beg);
    trtModelStream = new char[size];
    assert(trtModelStream);
    file.read(trtModelStream, size);
    file.close();
    IRuntime* runtime = createInferRuntime(gLogger);
    assert(runtime != nullptr);
    ICudaEngine* engine = runtime->deserializeCudaEngine(trtModelStream, size);
    assert(engine != nullptr);
    IExecutionContext* context = engine->createExecutionContext();
    assert(context != nullptr);
    delete[] trtModelStream;
    assert(engine->getNbBindings() == 2);
    CUDA_CHECK(cudaMalloc(&buffers[inputIndex], BATCH_SIZE * 3 * INPUT_H * INPUT_W * sizeof(float)));
    CUDA_CHECK(cudaMalloc(&buffers[outputIndex], BATCH_SIZE * OUTPUT_SIZE * sizeof(float)));
}


void YOLOV5::forward(const cv::Mat &img, vector<Object>& results){
    cv::Mat pr_img = preprocess_img(img, INPUT_W, INPUT_H); // letterbox BGR to RGB
    // todo edit here 
    int i = 0;
        for (int row = 0; row < INPUT_H; ++row) {
            uchar* uc_pixel = pr_img.data + row * pr_img.step;
            for (int col = 0; col < INPUT_W; ++col) {
                data[b * 3 * INPUT_H * INPUT_W + i] = (float)uc_pixel[2] / 255.0;
                data[b * 3 * INPUT_H * INPUT_W + i + INPUT_H * INPUT_W] = (float)uc_pixel[1] / 255.0;
                data[b * 3 * INPUT_H * INPUT_W + i + 2 * INPUT_H * INPUT_W] = (float)uc_pixel[0] / 255.0;
                uc_pixel += 3;
                ++i;
            }
        }
    }

    // Run inference
    auto start = std::chrono::system_clock::now();
    // DMA input batch data to device, infer on the batch asynchronously, and DMA output back to host
    CUDA_CHECK(cudaMemcpyAsync(buffers[0], data, batchSize * 3 * INPUT_H * INPUT_W * sizeof(float), cudaMemcpyHostToDevice, stream));
    context.enqueue(batchSize, buffers, stream, nullptr);
    CUDA_CHECK(cudaMemcpyAsync(prob, buffers[1], batchSize * OUTPUT_SIZE * sizeof(float), cudaMemcpyDeviceToHost, stream));
    cudaStreamSynchronize(stream);
    auto end = std::chrono::system_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    
    std::vector<Yolo::Detection> res;
    nms(res, &prob[0 * OUTPUT_SIZE], mThreshold, NMS_THRESH);
    for(auto r:res){
        results.push_back({r.conf, mLabels[r.class_id], get_rect(img, r.bbox)});
    }
}

void YOLOV5::uninit(){
    // Remove labels
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
    // Release stream and buffers
    cudaStreamDestroy(stream);
    CUDA_CHECK(cudaFree(buffers[inputIndex]));
    CUDA_CHECK(cudaFree(buffers[outputIndex]));
    // Destroy the engine
    context->destroy();
    engine->destroy();
    runtime->destroy();
}

