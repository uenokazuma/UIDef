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
        //for (auto* t : pool) TF_DeleteTensor(t);
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

class TFModel {
    TF_Session* session;
    TF_Graph* graph;
    TF_Status* status;
    TF_Output input_op;
    TF_Output output_op;

public:
    TFModel(const char* model_dir) {
        status = TF_NewStatus();
        graph = TF_NewGraph();

        // Create session options
        TF_SessionOptions* sess_opts = TF_NewSessionOptions();

        // Load saved model
        const char* tags[] = { "serve" };
        int ntags = 1;
        TF_Buffer* run_options = TF_NewBufferFromString("", 0);
        TF_Buffer* meta_graph_def = TF_NewBuffer();

        session = TF_LoadSessionFromSavedModel(sess_opts, run_options, model_dir,
            tags, ntags, graph, meta_graph_def, status);

        if (TF_GetCode(status) != TF_OK) {
            //std::cerr << "Error loading model: " << TF_Message(status) << std::endl;
            exit(1);
        }

        // Get input/output operations
        input_op = { TF_GraphOperationByName(graph, "serving_default_keras_tensor_121"), 0 };
        output_op = { TF_GraphOperationByName(graph, "StatefulPartitionedCall_1"), 0 };

        TF_DeleteSessionOptions(sess_opts);
        TF_DeleteBuffer(run_options);
        TF_DeleteBuffer(meta_graph_def);
    }

    ~TFModel() {
        TF_CloseSession(session, status);
        TF_DeleteSession(session, status);
        TF_DeleteGraph(graph);
        TF_DeleteStatus(status);
    }

    std::vector<float> predict_batch(const std::vector<std::vector<float>>& batch) {
        TF_Tensor* input_tensor = tensor_pool.get(batch.size());

        float* data = static_cast<float*>(TF_TensorData(input_tensor));
        for (const auto& img : batch) {
            std::memcpy(data, img.data(), img.size() * sizeof(float));
            data += img.size();
        }

        TF_Tensor* output_tensor = nullptr;
        TF_Output inputs[1] = { input_op };
        TF_Tensor* input_tensors[1] = { input_tensor };
        TF_Output outputs[1] = { output_op };
        TF_Tensor* output_tensors[1] = { nullptr };

        TF_SessionRun(session, nullptr,
            inputs, input_tensors, 1,
            outputs, output_tensors, 1,
            nullptr, 0, nullptr, status);


        if (TF_GetCode(status) != TF_OK) {
            //std::cerr << "Error running session: " << TF_Message(status) << std::endl;
            TF_DeleteTensor(input_tensor);
            return {};
        }

        // Process output
        float* out_vals = static_cast<float*>(TF_TensorData(output_tensors[0]));
        size_t num_results = TF_TensorElementCount(output_tensors[0]);
        std::vector<float> results(out_vals, out_vals + num_results);

        tensor_pool.put(input_tensor);
        return results;
    }
};

class FileProcessor {
    static constexpr size_t CHUNK_SIZE = 1024;
    static constexpr size_t IMG_SIZE = 256;

    // Precomputed Viridis colormap LUT [R, G, B, A]
    static constexpr uint8_t VIRIDIS_LUT[256][4] = {
    {68,1,84,255},
    {68,2,85,255},
    {68,3,87,255},
    {69,5,88,255},
    {69,6,90,255},
    {69,8,91,255},
    {70,9,92,255},
    {70,11,94,255},
    {70,12,95,255},
    {70,14,97,255},
    {71,15,98,255},
    {71,17,99,255},
    {71,18,101,255},
    {71,20,102,255},
    {71,21,103,255},
    {71,22,105,255},
    {71,24,106,255},
    {72,25,107,255},
    {72,26,108,255},
    {72,28,110,255},
    {72,29,111,255},
    {72,30,112,255},
    {72,32,113,255},
    {72,33,114,255},
    {72,34,115,255},
    {72,35,116,255},
    {71,37,117,255},
    {71,38,118,255},
    {71,39,119,255},
    {71,40,120,255},
    {71,42,121,255},
    {71,43,122,255},
    {71,44,123,255},
    {70,45,124,255},
    {70,47,124,255},
    {70,48,125,255},
    {70,49,126,255},
    {69,50,127,255},
    {69,52,127,255},
    {69,53,128,255},
    {69,54,129,255},
    {68,55,129,255},
    {68,57,130,255},
    {67,58,131,255},
    {67,59,131,255},
    {67,60,132,255},
    {66,61,132,255},
    {66,62,133,255},
    {66,64,133,255},
    {65,65,134,255},
    {65,66,134,255},
    {64,67,135,255},
    {64,68,135,255},
    {63,69,135,255},
    {63,71,136,255},
    {62,72,136,255},
    {62,73,137,255},
    {61,74,137,255},
    {61,75,137,255},
    {61,76,137,255},
    {60,77,138,255},
    {60,78,138,255},
    {59,80,138,255},
    {59,81,138,255},
    {58,82,139,255},
    {58,83,139,255},
    {57,84,139,255},
    {57,85,139,255},
    {56,86,139,255},
    {56,87,140,255},
    {55,88,140,255},
    {55,89,140,255},
    {54,90,140,255},
    {54,91,140,255},
    {53,92,140,255},
    {53,93,140,255},
    {52,94,141,255},
    {52,95,141,255},
    {51,96,141,255},
    {51,97,141,255},
    {50,98,141,255},
    {50,99,141,255},
    {49,100,141,255},
    {49,101,141,255},
    {49,102,141,255},
    {48,103,141,255},
    {48,104,141,255},
    {47,105,141,255},
    {47,106,141,255},
    {46,107,142,255},
    {46,108,142,255},
    {46,109,142,255},
    {45,110,142,255},
    {45,111,142,255},
    {44,112,142,255},
    {44,113,142,255},
    {44,114,142,255},
    {43,115,142,255},
    {43,116,142,255},
    {42,117,142,255},
    {42,118,142,255},
    {42,119,142,255},
    {41,120,142,255},
    {41,121,142,255},
    {40,122,142,255},
    {40,122,142,255},
    {40,123,142,255},
    {39,124,142,255},
    {39,125,142,255},
    {39,126,142,255},
    {38,127,142,255},
    {38,128,142,255},
    {38,129,142,255},
    {37,130,142,255},
    {37,131,141,255},
    {36,132,141,255},
    {36,133,141,255},
    {36,134,141,255},
    {35,135,141,255},
    {35,136,141,255},
    {35,137,141,255},
    {34,137,141,255},
    {34,138,141,255},
    {34,139,141,255},
    {33,140,141,255},
    {33,141,140,255},
    {33,142,140,255},
    {32,143,140,255},
    {32,144,140,255},
    {32,145,140,255},
    {31,146,140,255},
    {31,147,139,255},
    {31,148,139,255},
    {31,149,139,255},
    {31,150,139,255},
    {30,151,138,255},
    {30,152,138,255},
    {30,153,138,255},
    {30,153,138,255},
    {30,154,137,255},
    {30,155,137,255},
    {30,156,137,255},
    {30,157,136,255},
    {30,158,136,255},
    {30,159,136,255},
    {30,160,135,255},
    {31,161,135,255},
    {31,162,134,255},
    {31,163,134,255},
    {32,164,133,255},
    {32,165,133,255},
    {33,166,133,255},
    {33,167,132,255},
    {34,167,132,255},
    {35,168,131,255},
    {35,169,130,255},
    {36,170,130,255},
    {37,171,129,255},
    {38,172,129,255},
    {39,173,128,255},
    {40,174,127,255},
    {41,175,127,255},
    {42,176,126,255},
    {43,177,125,255},
    {44,177,125,255},
    {46,178,124,255},
    {47,179,123,255},
    {48,180,122,255},
    {50,181,122,255},
    {51,182,121,255},
    {53,183,120,255},
    {54,184,119,255},
    {56,185,118,255},
    {57,185,118,255},
    {59,186,117,255},
    {61,187,116,255},
    {62,188,115,255},
    {64,189,114,255},
    {66,190,113,255},
    {68,190,112,255},
    {69,191,111,255},
    {71,192,110,255},
    {73,193,109,255},
    {75,194,108,255},
    {77,194,107,255},
    {79,195,105,255},
    {81,196,104,255},
    {83,197,103,255},
    {85,198,102,255},
    {87,198,101,255},
    {89,199,100,255},
    {91,200,98,255},
    {94,201,97,255},
    {96,201,96,255},
    {98,202,95,255},
    {100,203,93,255},
    {103,204,92,255},
    {105,204,91,255},
    {107,205,89,255},
    {109,206,88,255},
    {112,206,86,255},
    {114,207,85,255},
    {116,208,84,255},
    {119,208,82,255},
    {121,209,81,255},
    {124,210,79,255},
    {126,210,78,255},
    {129,211,76,255},
    {131,211,75,255},
    {134,212,73,255},
    {136,213,71,255},
    {139,213,70,255},
    {141,214,68,255},
    {144,214,67,255},
    {146,215,65,255},
    {149,215,63,255},
    {151,216,62,255},
    {154,216,60,255},
    {157,217,58,255},
    {159,217,56,255},
    {162,218,55,255},
    {165,218,53,255},
    {167,219,51,255},
    {170,219,50,255},
    {173,220,48,255},
    {175,220,46,255},
    {178,221,44,255},
    {181,221,43,255},
    {183,221,41,255},
    {186,222,39,255},
    {189,222,38,255},
    {191,223,36,255},
    {194,223,34,255},
    {197,223,33,255},
    {199,224,31,255},
    {202,224,30,255},
    {205,224,29,255},
    {207,225,28,255},
    {210,225,27,255},
    {212,225,26,255},
    {215,226,25,255},
    {218,226,24,255},
    {220,226,24,255},
    {223,227,24,255},
    {225,227,24,255},
    {228,227,24,255},
    {231,228,25,255},
    {233,228,25,255},
    {236,228,26,255},
    {238,229,27,255},
    {241,229,28,255},
    {243,229,30,255},
    {246,230,31,255},
    {248,230,33,255},
    {250,230,34,255},
    {253,231,36,255}
    };

    // Reusable buffers to prevent reallocations
    struct ThreadLocalBuffers {
        std::vector<uint8_t> byte_buffer;
        std::vector<uint8_t> heatmap_buffer;
        std::vector<float> output_buffer;

        ThreadLocalBuffers() :
            byte_buffer(1024),
            heatmap_buffer(32 * 32 * 4),
            output_buffer(256 * 256 * 3) {
        }
    };

    static thread_local ThreadLocalBuffers tls_buffers;

public:
    static const std::vector<float>& process_file(const std::string& path) {
        auto& buf = tls_buffers;

        // Read bytes directly into pre-allocated buffer
        std::ifstream file(path, std::ios::binary);
        file.read(reinterpret_cast<char*>(buf.byte_buffer.data()), 1024);

        // Process using pre-allocated buffers
        apply_colormap_normalized(buf.byte_buffer, buf.heatmap_buffer);
        preprocess_image(buf.heatmap_buffer, buf.output_buffer);

        return buf.output_buffer;
    }

private:
    static void apply_colormap_normalized(const std::vector<uint8_t>& bytes,
        std::vector<uint8_t>& heatmap) {
        constexpr size_t num_pixels = 32 * 32;
        for (size_t i = 0; i < num_pixels; ++i) {
            const uint8_t val = bytes[i];
            const auto& color = VIRIDIS_LUT[val];
            heatmap[i * 4] = color[0];
            heatmap[i * 4 + 1] = color[1];
            heatmap[i * 4 + 2] = color[2];
            heatmap[i * 4 + 3] = color[3];
        }
    }

    static void preprocess_image(const std::vector<uint8_t>& heatmap,
        std::vector<float>& output) {
        constexpr float scale = 32.0f / 256.0f;
        constexpr size_t output_size = 256 * 256 * 3;

        output.resize(output_size);

        // Direct pointer access for speed
        float* out_ptr = output.data();

        for (size_t y = 0; y < 256; ++y) {
            for (size_t x = 0; x < 256; ++x) {
                const size_t src_x = static_cast<size_t>(x * scale);
                const size_t src_y = static_cast<size_t>(y * scale);
                const size_t src_idx = (src_y * 32 + src_x) * 4;

                *out_ptr++ = heatmap[src_idx] / 255.0f;     // R
                *out_ptr++ = heatmap[src_idx + 1] / 255.0f;   // G
                *out_ptr++ = heatmap[src_idx + 2] / 255.0f;   // B
            }
        }
    }
};

thread_local FileProcessor::ThreadLocalBuffers FileProcessor::tls_buffers;

std::string Visualization::scan(const std::string& filename) {

    std::string response = "0";
    std::string pathModel = File::getPathDir() + "\\data\\vism\\seq_binary_model";
    const char* argModel = pathModel.c_str();

    TFModel model(argModel);

    const size_t BATCH_SIZE = 64;
    std::vector<std::vector<float>> batch;
    std::vector<std::string> paths;

    try {
        auto processed = FileProcessor::process_file(filename);
        batch.push_back(processed);
        paths.push_back(filename);

        //if (batch.size() >= BATCH_SIZE) {
            auto results = model.predict_batch(batch);
            for (size_t j = 0; j < results.size(); ++j) {
                response = results[j] > 0.5 ? std::to_string(results[j]) : "0";
            }

            batch.clear();
            paths.clear();
        }
    //}
    catch (const std::exception& e) {

    }

    return response;
}