#include "Time.h"
#include <iostream>

std::string Time::timestamp() {
    struct tm localTime;
    struct timeb now;
    char timeChar[20];
    std::string timeString;
    
    ftime(&now);
    localtime_s(&localTime, &now.time);
    
    sprintf_s(timeChar, "%4d-%02d-%02d %02d:%02d:%02d",
        localTime.tm_year + 1900,
        localTime.tm_mon,
        localTime.tm_mday,
        localTime.tm_hour,
        localTime.tm_min,
        localTime.tm_sec);

    timeString = timeChar;

    return timeString;
}

std::string Time::timestampMillis() {
    //struct timeb now;
    std::string timeChar;
    /*std::string timeStamp = Time::timestamp();

    ftime(&now);

    std::sprintf(&timeChar, "%19s.%03d",
        &timeStamp,
        now.millitm
    );*/

    return timeChar;
}