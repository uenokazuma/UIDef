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