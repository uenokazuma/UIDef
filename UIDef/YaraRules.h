#pragma once
#include "File.h"
#include "Convert.h"
#include <filesystem>
#include <yara.h>

class YaraRules
{
public:
    static std::string scan(const std::string& filename);
};

