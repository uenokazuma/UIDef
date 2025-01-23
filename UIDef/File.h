#pragma once
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <shlobj.h>
#include <AclAPI.h>
#include <Sddl.h>
#include <openssl/evp.h>
#include <nlohmann/json.hpp>
//#include <openssl/err.h>

class File {
public:
    enum HashType { MD5, SHA1, SHA256 };
    static bool checkPath(const std::string& path);
    static std::string getPathDir();
    static bool createDir(const std::string& path);
    static bool ignoreFile(const std::string& filename);
    static void scan(const std::filesystem::path& path, std::vector<std::filesystem::path>& file);
    static std::string hash(const std::string& fileName, File::HashType type);
};