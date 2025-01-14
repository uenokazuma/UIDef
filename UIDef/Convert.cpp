#include "Convert.h"


std::string Convert::WCharToStr(const wchar_t* wstr) {
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &str[0], size, NULL, NULL);

    return str;
}