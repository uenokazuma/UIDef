#pragma once
#include "File.h"
#include "Convert.h"
#include <string>
#include <filesystem>
#include <yara.h>

class YaraRules
{
public:
    YaraRules();
    void compileRulesRecursive(const std::string& folder);
    bool scan(const std::string& filename);
    ~YaraRules();
private:
    YR_RULES* rules = nullptr;
};

