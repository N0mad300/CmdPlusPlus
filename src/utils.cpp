#include "utils.h"

#include <filesystem>
#include <sstream>
#include <fstream>
#include <thread>
#include <iostream>
#include <string>
#include <cctype>

#include <Windows.h>

using namespace fs;

std::string Utils::currentDirectory;

void Utils::initializeCurrentDirectory()
{
    currentDirectory = fs::current_path().string();
}

std::wstring Utils::stringToWstring(const std::string &narrowStr)
{
    std::wstringstream wss;
    wss << narrowStr.c_str();
    return wss.str();
}

std::wstring Utils::charToWchar(const WCHAR *str)
{
    // Create a wide string stream
    std::wstringstream wss;
    // Insert the narrow string into the stream
    wss << str;
    // Extract the wide string from the stream
    return wss.str();
}

std::wstring Utils::getProgramPath()
{
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring path(buffer);
    return path;
}

std::wstring Utils::getParentFolderPath(const std::wstring &path)
{
    std::filesystem::path p(path);
    return p.parent_path().wstring();
}

std::string Utils::getCurrentDir()
{
    return currentDirectory;
}

std::string Utils::wstringToString(const std::wstring &wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter; // Use the converter to convert wstring to string
    return converter.to_bytes(wstr);
}

const std::string Utils::licensePath = []()
{
    std::ostringstream oss;
    oss << wstringToString(getParentFolderPath(getProgramPath())) << "\\LICENSE.md";
    return oss.str();
}();

bool Utils::startsWithPeriod(const std::wstring &filename)
{
    // Check if the filename is not empty and if its first character is a period
    return !filename.empty() && filename[0] == L'.';
}

bool Utils::containsSpace(const std::string &str)
{
    for (char c : str)
    {
        if (c == ' ')
        {
            return true; // Found a space, return true
        }
    }
    return false; // No space found
}

int Utils::detectNumberBase(const std::string &str)
{
    if (str.empty())
    {
        return 0;
    }

    // Check for a possible sign at the beginning
    size_t startIdx = 0;
    if (str[0] == '-' || str[0] == '+')
    {
        startIdx = 1;
    }

    // Check if the string starts with "0x" or "0X" (hexadecimal)
    if (str.size() > startIdx + 1 && str[startIdx] == '0' &&
        (str[startIdx + 1] == 'x' || str[startIdx + 1] == 'X'))
    {
        return 16;
    }

    // Check if the string starts with "0" (octal)
    if (str.size() > startIdx && str[startIdx] == '0')
    {
        return 8;
    }

    // Check if the string is purely numeric (decimal)
    for (size_t i = startIdx; i < str.size(); ++i)
    {
        if (!std::isdigit(str[i]))
        {
            return 0;
        }
    }

    return 10;
}

void Utils::EnableDebugPrivileges()
{
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        TOKEN_PRIVILEGES tp;
        if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid))
        {
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            if (AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL))
            {
                if (GetLastError() != ERROR_NOT_ALL_ASSIGNED)
                {
                    // Successfully enabled debug privileges
                    std::cout << "Debug privileges successfully enabled." << std::endl;
                    CloseHandle(hToken);
                    return;
                }
                else
                {
                    std::cerr << "Failed to enable all privileges. Some privileges could not be enabled." << std::endl;
                }
            }
            else
            {
                std::cerr << "AdjustTokenPrivileges failed: " << GetLastError() << std::endl;
            }
        }
        else
        {
            std::cerr << "LookupPrivilegeValue failed: " << GetLastError() << std::endl;
        }
        CloseHandle(hToken);
    }
    else
    {
        std::cerr << "OpenProcessToken failed: " << GetLastError() << std::endl;
    }
}

void Utils::typeText(const std::string &filePath, int speed)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Error: Unable to open file " << filePath << std::endl;
        return;
    }

    char character;
    while (file.get(character))
    {
        std::cout << character << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(speed));
    }

    // Skip two lines
    std::cout << std::endl;
    std::cout << std::endl;

    file.close();
}