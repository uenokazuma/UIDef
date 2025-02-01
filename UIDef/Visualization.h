#pragma once
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <string>
#include <sys/stat.h>
#include <chrono>
#include <mutex>
#include "File.h"

#include <opencv2/opencv.hpp>
#include <tensorflow/c/c_api.h>

class TFModel {
    TF_Session* session;
    TF_Graph* graph;
    TF_Status* status;
    TF_Output input_op;
    TF_Output output_op;
    int input_height, input_width;

public:
    TFModel(const char* model_dir);
    ~TFModel();

    float predict(const cv::Mat& image);
    //std::vector<float> predict_batch(const std::vector<std::vector<float>>& batch);
};

class Visualization
{
public:
    static std::string scan(const std::string& filename, TFModel model);

private:
    //static cppflow::model model;
    //static void loadModel(const std::string& modelPath);
    //static std::string predict(const std::string& imagePath);
    //std::vector<float> extractBytes(const std::string& filePath, size_t length = 1024);
    //std::vector<float> normalize(const std::vector<float>& data);
};

