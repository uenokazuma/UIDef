#include "Visualization.h"
#include <regex>

//typedef std::string (*ProcessFile)(const std::string&);
//
//std::string Visualization::scan(const std::string& filename) {
//
//    HMODULE dllHandle = LoadLibrary(L"visualization.dll");
//    if (dllHandle == NULL) {
//        return "";
//    }
//
//    ProcessFile processFile = (ProcessFile)GetProcAddress(dllHandle, "processFile");
//
//    std::string response = "benign";
//    response = processFile(filename);
//
//    //std::string rulesDirectory = File::getPathDir() + "\\data\\vism";
//
//    //for (const auto& entry : std::filesystem::directory_iterator(rulesDirectory)) {
//    //    if (entry.is_regular_file() && entry.path().extension() == ".pb") {
//            //response = predict(filename, entry.path().string());
//            //response = predict(filename, rulesDirectory);
//    //    }
//    //}
//
//    return response;
//}

std::string Visualization::scan(const std::string& filename) {

    std::string response = "benign";
    std::string filepath = File::getPathDir();
    const char* filepathChar = filepath.c_str();
    const char* argImage = "data\\visc";
    const char* argModel = "data\\vism\\seq_binary_model.keras";
    const char* file = filename.c_str();
    //std::string command = "visualization_inference.exe " + file;
    char command[1024];
    
    sprintf_s(command, sizeof(command), "visualization_inference.exe \"%s\" --image_dir=%s\\%s --model=%s\\%s", file, filepathChar, argImage, filepathChar, argModel);

    STARTUPINFO startInfo = { 0 };
    PROCESS_INFORMATION processInfo;
    startInfo.cb = sizeof(startInfo);

    SECURITY_ATTRIBUTES secureAttr;
    secureAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    secureAttr.bInheritHandle = TRUE;
    secureAttr.lpSecurityDescriptor = NULL;

    HANDLE hStdoutRead, hStdoutWrite;

    CreatePipe(&hStdoutRead, &hStdoutWrite, &secureAttr, 0);
    SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0);

    startInfo.dwFlags |= STARTF_USESTDHANDLES;
    startInfo.hStdOutput = hStdoutWrite;
    startInfo.hStdError = hStdoutWrite;

    std::string commandChar = command;
    LPWSTR commandLpwstr = Convert::StrToLPWSTR(commandChar);
    if (CreateProcess(NULL, commandLpwstr, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &startInfo, &processInfo)) {
        CloseHandle(hStdoutWrite);

        char receive[4096];
        DWORD bytesRead;
        std::stringstream outStream;

        while (ReadFile(hStdoutRead, receive, sizeof(receive), &bytesRead, NULL) && bytesRead > 0) {
            outStream.write(receive, bytesRead);
        }

        std::regex regex("Probability:\\s*(\\d+\\.\\d+)");
        std::smatch match;
        std::string receiveSearch = receive;

        if (std::regex_search(receiveSearch, match, regex)) {
            response = match[1];
        }

        //WaitForSingleObject(processInfo.hProcess, INFINITE);

        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        CloseHandle(hStdoutRead);
    }

    return response;
}