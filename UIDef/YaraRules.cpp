#include "YaraRules.h"

std::string YaraRules::scan(const std::string& filename) {

    std::string response = "no";
    std::string rulesDirectory = File::getPathDir() + "\\data\\yru";
    

    for (const auto& entry : std::filesystem::directory_iterator(rulesDirectory)) {
        if (yr_initialize() != ERROR_SUCCESS) {
            return "";
        }

        YR_COMPILER* compiler;
        YR_RULES* rules = NULL;
        if (yr_compiler_create(&compiler) != ERROR_SUCCESS) {
            yr_finalize();
        }

        if (entry.is_regular_file() && entry.path().extension() == ".yar") {

            std::string pathString = entry.path().string();
            std::string filenameString = entry.path().filename().string();
            const char* pathChar = pathString.c_str();
            const char* filenameChar = filenameString.c_str();
            FILE* fileRule;
            errno_t errFileRule = fopen_s(&fileRule, pathChar, "r");
            if (errFileRule != 0) {
                yr_compiler_destroy(compiler);
                yr_finalize();
                continue;
            }

            if (yr_compiler_add_file(compiler, fileRule, NULL, filenameChar) != ERROR_SUCCESS) {
                yr_compiler_destroy(compiler);
                yr_finalize();
                continue;
            }
        }
        
        if (yr_compiler_get_rules(compiler, &rules) != ERROR_SUCCESS) {
            yr_compiler_destroy(compiler);
            yr_finalize();
            return "";
        }

        yr_compiler_destroy(compiler);

        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            yr_rules_destroy(rules);
            yr_finalize();
            return "";
        }

        std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        uint8_t* fileContentUint8_t = Convert::CharToUint8_t(fileContent.data());
        int scanResult = yr_rules_scan_mem(rules, fileContentUint8_t, fileContent.size(), 0, NULL, NULL, NULL);
        if (scanResult == ERROR_SUCCESS) {
            response = "yes";
        }

        yr_rules_destroy(rules);

        yr_finalize();
    }

    return response;
}