#include "Visualization.h"

constexpr size_t BATCH_SIZE = 64;

class TensorPool {
    std::mutex mtx;
    std::vector<TF_Tensor*> pool;
    size_t element_size;

public:
    TensorPool(size_t element_size) : element_size(element_size) {}

    TF_Tensor* get(size_t batch_size) {
        std::lock_guard<std::mutex> lock(mtx);
        if (!pool.empty()) {
            auto* tensor = pool.back();
            if (TF_Dim(tensor, 0) != batch_size) {
                TF_DeleteTensor(tensor);
                pool.pop_back();
                return create_new(batch_size);
            }
            return tensor;
        }
        return create_new(batch_size);
    }

    void put(TF_Tensor* tensor) {
        std::lock_guard<std::mutex> lock(mtx);
        pool.push_back(tensor);
    }

    ~TensorPool() {
        for (auto* t : pool) TF_DeleteTensor(t);
    }
private:
    TF_Tensor* create_new(size_t batch_size) {
        const int64_t dims[] = { static_cast<int64_t>(batch_size), 256, 256, 3 };
        return TF_AllocateTensor(TF_FLOAT, dims, 4,
            batch_size * 256 * 256 * 3 * sizeof(float));
    }
};

// Global pool for 256x256x3 float tensors
TensorPool tensor_pool(256 * 256 * 3 * sizeof(float));


TFModel::TFModel(const char* model_dir) {
    status = TF_NewStatus();
    graph = TF_NewGraph();

    // Create session options
    TF_SessionOptions* sess_opts = TF_NewSessionOptions();

    // Load saved model
    const char* tags[] = { "serve" };
    int ntags = 1;
    //TF_Buffer* run_options = TF_NewBufferFromString("", 0);
    TF_Buffer* run_options = TF_NewBuffer();
    TF_Buffer* meta_graph_def = TF_NewBuffer();

    session = TF_LoadSessionFromSavedModel(sess_opts, run_options, model_dir,
        tags, ntags, graph, meta_graph_def, status);

    if (TF_GetCode(status) != TF_OK) {
        //std::cerr << "Error loading model: " << TF_Message(status) << std::endl;
        exit(1);
    }

    // Get input/output operations
    //input_op = { TF_GraphOperationByName(graph, "serving_default_keras_tensor_121"), 0 };
    //output_op = { TF_GraphOperationByName(graph, "StatefulPartitionedCall_1"), 0 };
    input_op = { TF_GraphOperationByName(graph, "serving_default_keras_tensor_121"), 0 };
    const TF_DataType type = TF_OperationOutputType(input_op);
    const int num_dims = TF_GraphGetTensorNumDims(graph, input_op, status);
    std::vector<int64_t> dims(num_dims);
    TF_GraphGetTensorShape(graph, input_op, dims.data(), num_dims, status);
    input_height = dims[1];
    input_width = dims[2];

    output_op = { TF_GraphOperationByName(graph, "StatefulPartitionedCall_1"), 0 };

    TF_DeleteSessionOptions(sess_opts);
    TF_DeleteBuffer(run_options);
    TF_DeleteBuffer(meta_graph_def);
}

TFModel::~TFModel() {
    //TF_CloseSession(session, status);
    //TF_DeleteSession(session, status);
    //TF_DeleteGraph(graph);
    //TF_DeleteStatus(status);
}

float TFModel::predict(const cv::Mat& image) {

    cv::Mat processed;
    cv::resize(image, processed, cv::Size(input_width, input_height));
    processed.convertTo(processed, CV_32FC3, 1.0 / 255.0);

    const int64_t dims[] = { 1, input_height, input_width, 3 };
    TF_Tensor* input_tensor = TF_AllocateTensor(TF_FLOAT, dims, 4, input_height * input_width * 3 * sizeof(float));

    cv::Mat rgb;
    cv::cvtColor(processed, rgb, cv::COLOR_BGR2RGB);
    std::memcpy(TF_TensorData(input_tensor), rgb.data, TF_TensorByteSize(input_tensor));

    TF_Tensor* output_tensor = nullptr;
    //TF_Output inputs[1] = { input_op };
    //TF_Tensor* input_tensors[1] = { input_tensor };
    //TF_Output outputs[1] = { output_op };
    //TF_Tensor* output_tensors[1] = { nullptr };

    TF_SessionRun(session, nullptr,
        &input_op, &input_tensor, 1,
        &output_op, &output_tensor, 1,
        nullptr, 0, nullptr, status);


    if (TF_GetCode(status) != TF_OK) {
        //std::cerr << "Error running session: " << TF_Message(status) << std::endl;
        TF_DeleteTensor(input_tensor);
        return {};
    }

    // Process output
    float* out_vals = static_cast<float*>(TF_TensorData(output_tensor));
    //size_t num_results = TF_TensorElementCount(output_tensor);
    float results = out_vals[0];

    TF_DeleteTensor(input_tensor);
    return results;
}

class FileProcessor {
    static constexpr size_t CHUNK_SIZE = 1024;
    static constexpr size_t IMG_SIZE = 256;

    // Precomputed Viridis colormap LUT [R, G, B, A]
    static constexpr uint8_t VIRIDIS_LUT[256][4] = {
    {68, 1, 84, 255},
    {68, 2, 86, 255},
    {69, 4, 87, 255},
    {69, 5, 89, 255},
    {69, 7, 90, 255},
    {70, 8, 92, 255},
    {70, 10, 93, 255},
    {70, 11, 94, 255},
    {70, 13, 96, 255},
    {71, 14, 97, 255},
    {71, 16, 99, 255},
    {71, 17, 100, 255},
    {71, 19, 101, 255},
    {71, 20, 103, 255},
    {71, 22, 104, 255},
    {71, 23, 105, 255},
    {71, 24, 106, 255},
    {71, 26, 108, 255},
    {71, 27, 109, 255},
    {71, 28, 110, 255},
    {71, 30, 111, 255},
    {71, 31, 112, 255},
    {71, 32, 113, 255},  {71, 34, 114, 255},
    {71, 35, 115, 255},
    {71, 36, 116, 255},  {71, 38, 117, 255},  {71, 39, 118, 255},
    {71, 40, 119, 255},
    {71, 42, 120, 255},  {71, 43, 121, 255},  {70, 44, 122, 255},
    {70, 46, 123, 255},
    {70, 47, 124, 255},  {70, 48, 125, 255},  {70, 50, 126, 255},
    {70, 51, 127, 255},
    {69, 52, 128, 255},  {69, 54, 129, 255},  {69, 55, 129, 255},
    {69, 56, 130, 255},
    {68, 57, 131, 255},  {68, 59, 132, 255},  {68, 60, 133, 255},
    {67, 61, 133, 255},
    {67, 62, 134, 255},  {67, 64, 135, 255},  {66, 65, 136, 255},
    {66, 66, 136, 255},
    {66, 67, 137, 255},  {65, 69, 138, 255},  {65, 70, 138, 255},
    {64, 71, 139, 255},
    {64, 72, 140, 255},  {63, 74, 140, 255},  {63, 75, 141, 255},
    {62, 76, 141, 255},
    {62, 77, 142, 255},  {61, 79, 143, 255},  {61, 80, 143, 255},
    {60, 81, 144, 255},
    {60, 82, 144, 255},  {59, 84, 145, 255},  {59, 85, 145, 255},
    {58, 86, 146, 255},
    {58, 87, 146, 255},  {57, 89, 147, 255},  {57, 90, 147, 255},
    {56, 91, 148, 255},
    {56, 92, 148, 255},  {55, 94, 149, 255},  {55, 95, 149, 255},
    {54, 96, 149, 255},
    {54, 97, 150, 255},  {53, 99, 150, 255},  {53, 100, 151, 255},
    {52, 101, 151, 255}, {52, 102, 151, 255}, {51, 104, 152, 255}, {51, 105, 152, 255},
    {50, 106, 152, 255}, {50, 107, 153, 255}, {49, 109, 153, 255}, {49, 110, 153, 255},
    {48, 111, 154, 255}, {48, 112, 154, 255}, {47, 114, 154, 255}, {47, 115, 154, 255},
    {46, 116, 155, 255}, {46, 117, 155, 255}, {45, 119, 155, 255}, {45, 120, 155, 255},
    {44, 121, 156, 255}, {44, 122, 156, 255}, {43, 124, 156, 255}, {43, 125, 156, 255},
    {42, 126, 156, 255}, {42, 127, 157, 255}, {41, 129, 157, 255}, {41, 130, 157, 255},
    {40, 131, 157, 255}, {40, 132, 157, 255}, {39, 134, 157, 255}, {39, 135, 158, 255},
    {38, 136, 158, 255}, {38, 137, 158, 255}, {37, 139, 158, 255}, {37, 140, 158, 255},
    {36, 141, 158, 255}, {36, 142, 158, 255}, {35, 144, 158, 255}, {35, 145, 158, 255},
    {34, 146, 158, 255}, {34, 147, 158, 255}, {33, 149, 158, 255}, {33, 150, 158, 255},
    {32, 151, 158, 255}, {32, 152, 158, 255}, {31, 154, 158, 255}, {31, 155, 158, 255},
    {31, 156, 158, 255}, {30, 157, 158, 255}, {30, 159, 158, 255}, {30, 160, 158, 255},
    {29, 161, 158, 255}, {29, 162, 158, 255}, {29, 164, 158, 255}, {28, 165, 158, 255},
    {28, 166, 158, 255}, {28, 167, 158, 255}, {27, 169, 158, 255}, {27, 170, 158, 255},
    {27, 171, 158, 255}, {26, 172, 158, 255}, {26, 174, 158, 255}, {26, 175, 158, 255},
    {25, 176, 158, 255}, {25, 177, 158, 255}, {25, 178, 158, 255}, {24, 180, 158, 255},
    {24, 181, 158, 255}, {24, 182, 158, 255}, {23, 183, 158, 255}, {23, 185, 158, 255},
    {23, 186, 158, 255}, {22, 187, 158, 255}, {22, 188, 158, 255}, {22, 189, 158, 255},
    {21, 191, 158, 255}, {21, 192, 158, 255}, {21, 193, 158, 255}, {20, 194, 158, 255},
    {20, 195, 158, 255}, {20, 197, 158, 255}, {19, 198, 158, 255}, {19, 199, 158, 255},
    {19, 200, 158, 255}, {18, 201, 158, 255}, {18, 203, 158, 255}, {18, 204, 158, 255},
    {17, 205, 158, 255}, {17, 206, 158, 255}, {17, 207, 158, 255}, {16, 209, 158, 255},
    {16, 210, 158, 255}, {16, 211, 158, 255}, {15, 212, 158, 255}, {15, 213, 158, 255},
    {15, 215, 158, 255}, {14, 216, 158, 255}, {14, 217, 158, 255}, {14, 218, 158, 255},
    {13, 219, 158, 255}, {13, 220, 158, 255}, {13, 222, 158, 255}, {12, 223, 158, 255},
    {12, 224, 158, 255}, {12, 225, 158, 255}, {11, 226, 158, 255}, {11, 227, 158, 255},
    {11, 229, 158, 255}, {10, 230, 158, 255}, {10, 231, 158, 255}, {10, 232, 158, 255},
    {9, 233, 158, 255},  {9, 234, 158, 255},  {9, 236, 158, 255},  {8, 237, 158, 255},
    {8, 238, 158, 255},  {8, 239, 158, 255},  {7, 240, 158, 255},  {7, 241, 158, 255},
    {7, 242, 158, 255},  {6, 244, 158, 255},  {6, 245, 158, 255},  {6, 246, 158, 255},
    {5, 247, 158, 255},  {5, 248, 158, 255},  {5, 249, 158, 255},  {4, 250, 158, 255},
    {4, 251, 158, 255},  {4, 253, 158, 255},  {3, 254, 158, 255},
    {3, 255, 158, 255} };

    // Reusable buffers to prevent reallocations
    struct ThreadLocalBuffers {
        std::vector<uint8_t> byte_buffer;
        cv::Mat heatmap_buffer;
        cv::Mat output_buffer;

        ThreadLocalBuffers() :
            byte_buffer(1024, 0),
            heatmap_buffer(32, 32, CV_8UC4) {
        }
    };

    static thread_local ThreadLocalBuffers tls_buffers;

public:
    static const cv::Mat process_file(const std::string& path) {
        std::vector<uint8_t> bytes(1024, 0);
        std::string filename = File::getFileName(path);
        // Read bytes directly into pre-allocated buffer
        std::ifstream file(path, std::ios::binary);
        file.read(reinterpret_cast<char*>(bytes.data()), 1024);

        // Process using pre-allocated buffers
        cv::Mat heatmap = apply_colormap_normalized(bytes);
        cv::Mat output = preprocess_image(filename, heatmap);

        return output;
    }

private:
    static cv::Mat apply_colormap_normalized(std::vector<uint8_t> bytes) {

        cv::Mat heatmap(32, 32, CV_8UC4);
        for (size_t i = 0; i < 1024; ++i) {
            const auto& color = VIRIDIS_LUT[bytes[i]];
            heatmap.data[i * 4] = color[0];
            heatmap.data[i * 4 + 1] = color[1];
            heatmap.data[i * 4 + 2] = color[2];
            heatmap.data[i * 4 + 3] = color[3];
        }

        return heatmap;
    }

    static cv::Mat preprocess_image(std::string filename, const cv::Mat& heatmap) {

        cv::Mat output;
        cv::resize(heatmap, output, cv::Size(291, 291), 0, 0, cv::INTER_NEAREST);
        //std::string imgPath = File::getPathDir() + "\\data\\visc\\" + filename + ".png";
        //cv::imwrite(imgPath, output);

        return output;
    }
};

thread_local FileProcessor::ThreadLocalBuffers FileProcessor::tls_buffers;

std::string Visualization::scan(const std::string& filename, TFModel model) {

    std::string response = "0";

    try {
        auto processed = FileProcessor::process_file(filename);
        auto results = model.predict(processed);
        response = results > 0.5 ? std::to_string(results) : "0";

    }
    catch (const std::exception& e) {

    }

    return response;
}