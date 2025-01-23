#include "YaraRules.h"

YaraRules::YaraRules() {
    auto err_code = yr_initialize();
    if (err_code != ERROR_SUCCESS) {
        throw std::runtime_error("Error initializing yara: " + std::to_string(err_code));
    }
}

YaraRules::~YaraRules() {
    if (rules != nullptr) {
        yr_rules_destroy(rules);
    }
    yr_finalize();
}

void YaraRules::compileRulesRecursive(const std::string& folder) {
    YR_COMPILER* compiler;
    auto err_code = yr_compiler_create(&compiler);
    if (err_code != ERROR_SUCCESS) {
        throw std::runtime_error("Error creating yara compiler: " + std::to_string(err_code));
    }
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(folder)) {
            if (entry.is_regular_file() && entry.path().extension() == ".yar") {
                FILE* fileRule;
                auto path = entry.path().string();
                auto errFileRule = fopen_s(&fileRule, path.c_str(), "r");
                if (errFileRule != 0) {
                    // TODO: proper error handling
                    continue;
                }
                if (yr_compiler_add_file(compiler, fileRule, path.c_str(), path.c_str()) != 0) {
                    // TODO: proper error handling
                    continue;
                }
                fclose(fileRule);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    err_code = yr_compiler_get_rules(compiler, &rules);
    if (err_code != ERROR_SUCCESS) {
        throw std::runtime_error("Error getting rules from compiler: " + std::to_string(err_code));
    }
    yr_compiler_destroy(compiler);
}

bool YaraRules::scan(const std::string& filename) {
    auto result = false;
    auto callback = [](YR_SCAN_CONTEXT* context, int message, void* message_data, void* user_data) {
        if (message == CALLBACK_MSG_RULE_MATCHING) {
            auto result = static_cast<bool*>(user_data);
            *result = true;
            return CALLBACK_ABORT;
        }
        return CALLBACK_CONTINUE;
        };

    auto err_code = yr_rules_scan_file(rules, filename.c_str(), SCAN_FLAGS_REPORT_RULES_MATCHING, callback, &result, 0);
    if (err_code != ERROR_SUCCESS) {
        throw std::runtime_error("Error scanning file: " + std::to_string(err_code));
    }
    return result;
}

//bool YaraRules::scan(const std::string& filename, const std::string& yaraRulePath) {
//
//    //std::string response = "no";
//    //std::string rulesDirectory = File::getPathDir() + "\\data\\yru";
//
//    //if (!std::filesystem::exists(rulesDirectory)) {
//    //    return "";
//    //}
//    
//
//    for (const auto& entry : std::filesystem::directory_iterator(rulesDirectory)) {
//        if (yr_initialize() != ERROR_SUCCESS) {
//            return "";
//        }
//
//        YR_COMPILER* compiler;
//        YR_RULES* rules = NULL;
//        if (yr_compiler_create(&compiler) != ERROR_SUCCESS) {
//            yr_finalize();
//        }
//
//        if (entry.is_regular_file() && entry.path().extension() == ".yar") {
//
//            std::string pathString = entry.path().string();
//            std::string filenameString = entry.path().filename().string();
//            const char* pathChar = pathString.c_str();
//            const char* filenameChar = filenameString.c_str();
//            FILE* fileRule;
//            errno_t errFileRule = fopen_s(&fileRule, pathChar, "r");
//            if (errFileRule != 0) {
//                yr_compiler_destroy(compiler);
//                yr_finalize();
//                continue;
//            }
//
//            if (yr_compiler_add_file(compiler, fileRule, NULL, filenameChar) != ERROR_SUCCESS) {
//                yr_compiler_destroy(compiler); 
//                yr_finalize();
//                continue;
//            }
//        }
//        
//        if (yr_compiler_get_rules(compiler, &rules) != ERROR_SUCCESS) {
//            yr_compiler_destroy(compiler);
//            yr_finalize();
//            return "";
//        }
//
//        yr_compiler_destroy(compiler);
//
//        std::ifstream file(filename, std::ios::binary);
//        if (!file.is_open()) {
//            yr_rules_destroy(rules);
//            yr_finalize();
//            return "";
//        }
//
//        std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
//        uint8_t* fileContentUint8_t = Convert::CharToUint8_t(fileContent.data());
//        int scanResult = yr_rules_scan_mem(rules, fileContentUint8_t, fileContent.size(), 0, NULL, NULL, NULL);
//        if (scanResult == ERROR_SUCCESS) {
//            response = "yes";
//        }
//
//        yr_rules_destroy(rules);
//
//        yr_finalize();
//    }
//
//    return response;
//}