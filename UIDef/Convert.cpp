#include "Convert.h"

std::wstring Convert::IntToWstr(int number) {

    std::wstring wideStr = std::to_wstring(number);

    return wideStr;
}

std::wstring Convert::StrToWstr(const std::string str) {
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);

    return wstr;
}

std::string Convert::WCharToStr(const wchar_t* wstr) {
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &str[0], size, NULL, NULL);

    return str;
}

char* Convert::WCharToChar(const wchar_t* wstr) {
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* chr = new char[size];
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, chr, size, NULL, NULL);

    return chr;
}

std::uint8_t* Convert::CharToUint8_t(const char* str) {
    uint8_t* char_uint8_t = (uint8_t*)str;

    return char_uint8_t;
}