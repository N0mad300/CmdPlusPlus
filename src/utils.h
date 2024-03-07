#ifndef UTILS
#define UTILS

#include <filesystem>
#include <cstring>
#include <cwchar>
#include <string>

#include <Windows.h>

namespace fs = std::filesystem;

namespace Utils
{
    extern std::string currentDirectory;
    extern const std::string licensePath;

    void typeText(const std::string &filePath, int speed);
    void initializeCurrentDirectory();
    void EnableDebugPrivileges();

    std::wstring stringToWstring(const std::string &narrowStr);
    std::wstring getParentFolderPath(const std::wstring &path);
    std::wstring charToWchar(const WCHAR *str);
    std::wstring getProgramPath();

    std::string getCurrentDir();
    std::string wstringToString(const std::wstring &wstr);

    bool startsWithPeriod(const std::wstring &filename);
    bool containsSpace(const std::string &str);

    int detectNumberBase(const std::string &str);
}

#endif