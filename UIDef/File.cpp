#include "File.h"

std::wstring getCurrentUser() {
    char username[256];
    std::wstring wstr;
    DWORD username_len = sizeof(username);
    if (GetUserNameA(username, &username_len)) {
        wstr = std::wstring(username, username + username_len - 1);
    }

    return wstr;
}

bool setFolderPermissions(const std::string& path) {

    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pOldDACL = NULL;
    std::wstring folderPath(path.begin(), path.end());
    std::wstring currentUser;

    char username[256];
    DWORD username_len = sizeof(username);
    if (GetUserNameA(username, &username_len)) {
        currentUser = std::wstring(username, username + username_len - 1);
    }
    else {
        return false;
    }

    DWORD dwSidSize = 0, dwDomainSize = 0;
    SID_NAME_USE sidType;

    LookupAccountName(NULL, currentUser.c_str(), NULL, &dwSidSize, NULL, &dwDomainSize, &sidType);


    if (GetNamedSecurityInfoW(folderPath.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pOldDACL, NULL, &pSD) != ERROR_SUCCESS) {
        return false;
    }

    EXPLICIT_ACCESS ea;
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE;
    ea.grfAccessMode = GRANT_ACCESS;
    ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;

    PSID pSID = (PSID)LocalAlloc(LPTR, dwSidSize);
    if (pSID == NULL) {
        return false;
    }

    WCHAR* domainName = (WCHAR*)LocalAlloc(LPTR, dwDomainSize * sizeof(WCHAR));
    if (domainName == NULL) {
        LocalFree(pSID);
        return false;
    }

    if (!LookupAccountName(NULL, currentUser.c_str(), pSID, &dwSidSize, domainName, &dwDomainSize, &sidType)) {
        LocalFree(pSID);
        LocalFree(domainName);
        return false;
    }

    LocalFree(domainName);

    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.ptstrName = (LPTSTR)pSID;

    PACL pNewDACL = NULL;
    if (SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL) != ERROR_SUCCESS) {
        return false;
    }

    if (SetNamedSecurityInfoW((LPWSTR)folderPath.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pNewDACL, NULL) != ERROR_SUCCESS) {
        return false;
    }

    if (pSID) {
        LocalFree(pSID);
    }

    if (pSD) {
        LocalFree(pSD);
    }

    return true;
}

std::string toHexString(const unsigned char* hash, size_t hashLength) {
    std::ostringstream oss;
    for (size_t i = 0; i < hashLength; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return oss.str();
}

bool File::checkPath(const std::string& path) {
    DWORD dwAttribute = GetFileAttributesA(path.c_str());
    if (dwAttribute == INVALID_FILE_ATTRIBUTES) {
        return false;
    }
    else if (dwAttribute & FILE_ATTRIBUTE_DIRECTORY) {
        return true;
    }

    return false;
}

std::string File::getPathDir() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, sizeof(path));
    std::string fullpath(path);
    size_t pos = fullpath.find_last_of("\\/");
    std::string pathDir = fullpath.substr(0, pos);

    return pathDir;
}

bool File::createDir(const std::string& path) {

    if (SHCreateDirectoryExA(NULL, path.c_str(), NULL) == ERROR_SUCCESS) {
        if (setFolderPermissions(path)) {
            return true;
        }
    }

    return false;
}

void File::scan(const std::filesystem::path& path, std::vector<std::filesystem::path>& file) {

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            
            if (entry.path().string().find("C:/Program Files") != std::string::npos ||
                entry.path().string().find("C:/Windows") != std::string::npos) {
                continue;
            }

            if (std::filesystem::is_regular_file(entry)) {
                //preparation if there are file extensions that need to be avoided.
                //std::set<std::string> avoidExt = { ".lll", ".ddd" }
                //std::string extension = entry.path().extension().string();
                //if (avoidExt.find(extension) != avoidExt.end()) {
                //    continue;
                //}

                file.push_back(entry.path());
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

std::string File::hash(const std::string& fileName, File::HashType type) {
    
    std::ifstream file(fileName, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    
    const size_t hashFileSize = 1024 * 16;
    char hashFile[hashFileSize];

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLength = 0;

    EVP_MD_CTX* mdContext = EVP_MD_CTX_new();
    if (mdContext == NULL) {
        return "";
    }

    const EVP_MD* md = NULL;

    switch (type) {
    case File::HashType::MD5:
        md = EVP_md5();
        break;
    case File::HashType::SHA1:
        md = EVP_sha1();
        break;
    case File::HashType::SHA256:
        md = EVP_sha256();
        break;
    }

    if (!EVP_DigestInit_ex(mdContext, md, NULL)) {
        EVP_MD_CTX_free(mdContext);
        return "";
    }

    while (file.read(hashFile, hashFileSize)) {
        EVP_DigestUpdate(mdContext, hashFile, file.gcount());
    }

    EVP_DigestUpdate(mdContext, hashFile, file.gcount());
    
    if (EVP_DigestFinal_ex(mdContext, hash, &hashLength) != 1) {
        EVP_MD_CTX_free(mdContext);
    }

    return toHexString(hash, hashLength);
}