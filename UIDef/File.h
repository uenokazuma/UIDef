#include <iostream>
#include <filesystem>
#include <windows.h>
#include <shlobj.h>
#include <AclAPI.h>
#include <Sddl.h>

class File {
public:
    static bool checkPath(const std::string& path);
    static bool createDir(const std::string& path);
    static void scan(const std::filesystem::path& path, std::vector<std::filesystem::path>& file);
};