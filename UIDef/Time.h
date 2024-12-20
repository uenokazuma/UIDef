#pragma once
#include <string>
#include <ctime>
#include <sys/timeb.h>

class Time {
public:
    static std::string timestamp();
    static std::string timestampMillis();
};