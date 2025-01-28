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

#include <tensorflow/c/c_api.h>

class Visualization
{
public:
    static std::string scan(const std::string& filename);

private:
    //static cppflow::model model;
    //static void loadModel(const std::string& modelPath);
    //static std::string predict(const std::string& imagePath);
    //std::vector<float> extractBytes(const std::string& filePath, size_t length = 1024);
    //std::vector<float> normalize(const std::vector<float>& data);
};

