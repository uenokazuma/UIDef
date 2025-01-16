#pragma once

#include "framework.h"
#include "resource.h"
//#include "Convert.h"
#include "Time.h"
#include "Connection.h"
//#include "File.h"
#include "YaraRules.h"
#include <thread>
#include <nlohmann/json.hpp>

class FormView
{
public:
    static void show(HINSTANCE, HWND);
};

