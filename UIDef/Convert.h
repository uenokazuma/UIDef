#pragma once
#include <windows.h>
#include <iostream>
#include <string>

class Convert
{
public:
    static std::wstring IntToWstr(int number);
    static std::string WCharToStr(const wchar_t* wstr);
    static char* WCharToChar(const wchar_t* wstr);
    static std::wstring StrToWstr(const std::string wstr);
    static std::uint8_t* CharToUint8_t(const char* str);
};

