#pragma once

#include "framework.h"
#include "resource.h"
//#include "Convert.h"
#include "Time.h"
#include "Connection.h"
#include "File.h"
#include "Visualization.h"
#include "YaraRules.h"
#include <deque>
#include <future>
#include <thread>
#include <nlohmann/json.hpp>

#define MAX_CONCURRENT_TASKS 30
#define BATCH_SIZE 200

inline int countTrueHash = 0;
inline int countTrueYara = 0;
inline int countTrueVis = 0;
inline int countTrueBenign = 0;

class FormView
{
public:
    static void show(HINSTANCE, HWND);
//private:
};

