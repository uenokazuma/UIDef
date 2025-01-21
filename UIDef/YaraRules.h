#pragma once
#include <windows.h>
#include <filesystem>
#include "File.h"
#include "Convert.h"
#include <yara.h>

class YaraRules
{
public:
    static std::string scan(const std::string& filename);
};

