#include "Visualization.h"

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

void saveHeatMap(const cv::Mat& heatMap, const std::string& filePath) {

    cv::imwrite(filePath, heatMap);

}

void plotHeatMap(const std::vector<float>& features, const std::string& savePath) {
    
    cv::Mat heatMap(32, 32, CV_32F, const_cast<float*>(features.data()));

    cv::normalize(heatMap, heatMap, 0, 255, cv::NORM_MINMAX);
    cv::Mat coloredHeatMap;

    cv::applyColorMap(heatMap, coloredHeatMap, cv::COLORMAP_VIRIDIS);

    coloredHeatMap.convertTo(heatMap, CV_8U);
    saveHeatMap(coloredHeatMap, savePath);

}

std::string predict(const std::string& filePath, const std::string& modelPath) {

    std::string checkDirectory = File::getPathDir() + "\\data\\visc";
    
    cppflow::model model(modelPath);
    std::vector<float> fileBytes = extractBytes(filePath);
    std::vector<float> normalizedData = normalize(fileBytes);
    std::string heatMapPath = checkDirectory + "\\ch_heatmap.png";
    
    plotHeatMap(normalizedData, heatMapPath);

    cv::Mat img = cv::imread(heatMapPath, cv::COLORMAP_VIRIDIS);
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

std::string Visualization::scan(const std::string& filename) {

    std::string response = "benign";
    std::string rulesDirectory = File::getPathDir() + "\\data\\vism";

    //for (const auto& entry : std::filesystem::directory_iterator(rulesDirectory)) {
    //    if (entry.is_regular_file() && entry.path().extension() == ".pb") {
            //response = predict(filename, entry.path().string());
            response = predict(filename, rulesDirectory);
    //    }
    //}

    return response;
}