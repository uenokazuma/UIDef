#include "Visualization.h"

void checkStatus(TF_Status* status) {
    if (TF_GetCode(status) != TF_OK) {
        exit(1);
    }
}

TF_Graph* loadGraph(const std::string& modelPath) {

    TF_Status* status = TF_NewStatus();
    TF_SessionOptions* options = TF_NewSessionOptions();
    TF_Session* session = NULL;

    TF_SavedModel
}

std::vector<float> extractBytes(const std::string& filePath, size_t length = 1024) {
    
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        exit(1);
    }

    std::vector<char> buffer(length, 0);
    file.read(buffer.data(), length);

    std::vector<float> data(buffer.begin(), buffer.end());

    file.close();

    return data;

}

std::vector<float> normalize(const std::vector<float>& data) {
    
    std::vector<float> normalizedData(data.size());
    for (size_t i = 0; i < data.size(); i++) {
        normalizedData[i] = data[i] / 255.0f;
    }

    return normalizedData;
}

std::string predict(const std::string& filePath, const std::string& modelPath) {

    std::string checkDirectory = File::getPathDir() + "\\data\\visc";
    
    cppflow::model model(modelPath);
    std::vector<float> fileBytes = extractBytes(filePath);
    std::vector<float> normalizedData = normalize(fileBytes);
    std::string heatMapPath = checkDirectory + "\\ch_heatmap.png";
    
    cv::Mat img = cv::imread(heatMapPath, cv::IMREAD_GRAYSCALE); 
    if (img.empty()) {
        exit(1);
    }

    cv::resize(img, img, cv::Size(32, 32));
    img.convertTo(img, CV_32F, 1.0 / 255.0);

    std::vector<float> imageData(img.begin<float>(), img.end<float>());
    cppflow::tensor tensorImage(imageData, { 1, 32, 32, 1 });
    cppflow::tensor result = model(tensorImage);

    auto prediction = result.get_data<float>();
    std::string label = prediction[0] > 0.5 ? std::to_string(prediction[0]) : "benign";

    return label;
}

void saveHeatMap(const cv::Mat& heatMap, const std::string& filePath) {

    cv::imwrite(filePath, heatMap);

}

void plotHeatMap(const std::vector<float>& features, const std::string& savePath) {
    
    cv::Mat heatMap(32, 32, CV_32F, const_cast<float*>(features.data()));

    cv::normalize(heatMap, heatMap, 0, 255, cv::NORM_MINMAX);
    heatMap.convertTo(heatMap, CV_8U);
    saveHeatMap(heatMap, savePath);

}

std::string Visualization::scan(const std::string& filename) {

    std::string response = "benign";
    std::string rulesDirectory = File::getPathDir() + "\\data\\vism";

    for (const auto& entry : std::filesystem::directory_iterator(rulesDirectory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".keras") {
            response = predict(filename, entry.path().string());
        }
    }

    return response;
}