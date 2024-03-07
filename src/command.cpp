#include <filesystem>
#include <iterator>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <regex>

#ifdef _WIN32
#include <Windows.h>
#include <Shlwapi.h>
#include <Psapi.h>
#include <dbghelp.h>
#include <conio.h>
#include <tchar.h>
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "Shlwapi.lib")
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include "command.h"
#include "tokenizer.h"
#include "utils.h"

using namespace Tokenizer;
using namespace Utils;
using namespace fs;

void Exe::executeCommand(const std::wstring &command)
{
    STARTUPINFOW startupInfo = {sizeof(STARTUPINFOW)};
    PROCESS_INFORMATION processInfo;

    if (CreateProcessW(NULL, const_cast<LPWSTR>(command.c_str()), NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &startupInfo, &processInfo))
    {
        // Wait for the process to finish (optional)
        WaitForSingleObject(processInfo.hProcess, INFINITE);

        // Close process and thread handles
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    }
    else
    {
        std::cerr << "Failed to execute command: " << command.c_str() << std::endl;
    }
}

void EchoCommand::execute(const std::vector<Token> &arguments)
{
    for (const auto &arg : arguments)
    {
        std::cout << arg.value << " ";
    }
    std::cout << std::endl;
}

void CdCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() == 1)
    {
        changeDirectory(arguments[0].value);
    }
    else
    {
        std::cout << "Current directory: " << getCurrentDir() << std::endl;
        std::cerr << "Usage: cd <directory>" << std::endl;
    }
}

void CdCommand::changeDirectory(const std::string &newDir)
{
    fs::path currentPath = fs::current_path();
    fs::path newPath = currentPath / newDir;

    if (fs::exists(newPath) && fs::is_directory(newPath))
    {
        Utils::currentDirectory = newPath.string();
        fs::current_path(newPath);
    }
    else
    {
        std::cerr << "Invalid directory: " << newDir << std::endl;
    }
}

void AssocCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() != 1 && arguments[0].type != TokenType::ARGUMENT)
    {
        const std::string &fileExtensionStr = arguments[0].value;
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring fileExtension = converter.from_bytes(fileExtensionStr);

        const size_t bufferSize = 4096;
        wchar_t buffer[bufferSize];

        if (AssocQueryStringW(ASSOCF_INIT_IGNOREUNKNOWN, ASSOCSTR_COMMAND, fileExtension.c_str(), nullptr, buffer, (DWORD *)&bufferSize) == S_OK)
        {
            std::wcout << L"Command associated with " << fileExtension << L":\n";
            std::wcout << buffer << std::endl;
        }
        else
        {
            std::cerr << "Error retrieving file association for " << fileExtension.c_str() << std::endl;
        }
    }
    else
    {
        std::cerr << "Usage: assoc <file_extension>" << std::endl;
        return;
    }
}

void ClsCommand::execute(const std::vector<Token> &arguments)
{
    clearScreen();

    typeText(licensePath, 25);
}

void ClsCommand::clearScreen()
{
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

void CmdCommand::execute(const std::vector<Token> &arguments)
{
#ifdef _WIN32
    startNewProcess();
#else
    forkAndExec();
#endif
}

#ifdef _WIN32
void CmdCommand::startNewProcess()
{
    // Get the path of the current executable
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    // Create the command line for the new process
    std::wstring commandLine = exePath;

    // Set up the process information structure
    STARTUPINFOW startupInfo = {sizeof(STARTUPINFOW)};
    PROCESS_INFORMATION processInfo;

    // Set the STARTF_USESHOWWINDOW flag and specify how the window is displayed
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_SHOWNORMAL; // You can use SW_SHOW, SW_HIDE, SW_MINIMIZE, etc.

    // Create a new process
    if (CreateProcessW(nullptr, const_cast<LPWSTR>(commandLine.c_str()), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &startupInfo, &processInfo))
    {
        // Close handles to the new process and thread
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
    }
    else
    {
        std::cerr << "Failed to create a new process. Error code: " << GetLastError() << std::endl;
    }
}
#else
void CmdCommand::forkAndExec()
{
    pid_t childPid = fork();

    if (childPid == -1)
    {
        std::cerr << "Failed to fork a new process." << std::endl;
        return;
    }

    if (childPid == 0)
    {
        // Child process
        execl("./cmdplusplus", "cmdplusplus", nullptr); // Change to your executable name and path
        std::cerr << "Failed to execute a new process." << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process
        int status;
        waitpid(childPid, &status, 0);
    }
}
#endif

void ColorCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() == 1)
    {
        changeTextColor(arguments[0].value);
    }
    else
    {
        std::cerr << "Usage: color <hex_color>" << std::endl;
    }
}

void ColorCommand::changeTextColor(const std::string &color)
{
#ifdef _WIN32
    // Windows: SetConsoleTextAttribute
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (consoleHandle != INVALID_HANDLE_VALUE)
    {
        WORD attributes = 0;
        attributes |= parseHexColor(color) & 0xFF; // Get color from HEX value

        // Set color of existing content
        CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
        GetConsoleScreenBufferInfo(consoleHandle, &bufferInfo);
        COORD startPosition = {0, 0};
        DWORD written;
        FillConsoleOutputAttribute(consoleHandle, attributes, bufferInfo.dwSize.X * bufferInfo.dwSize.Y, startPosition, &written);

        // Set color of the new content
        SetConsoleTextAttribute(consoleHandle, attributes);
    }
#else
    // Linux/macOS: ANSI escape codes
    std::cout << "\033[38;2;" << parseHexColor(color) << "m";
#endif
}

int ColorCommand::parseHexColor(const std::string &hexColor)
{
    try
    {
        return std::stoi(hexColor, nullptr, 16);
    }
    catch (const std::invalid_argument &e)
    {
        std::cerr << "Invalid hex color format: " << hexColor << std::endl;
        return 0;
    }
}

void CompCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() == 2)
    {
        compFile(arguments[0].value, arguments[1].value);
    }
    else
    {
        std::cerr << "Usage: comp <file1> <file2>" << std::endl;
    }
}

void CompCommand::compFile(const std::string &filePath1, const std::string &filePath2)
{

    std::ifstream file1(filePath1, std::ios::binary);
    std::ifstream file2(filePath2, std::ios::binary);

    if (!file1.is_open() || !file2.is_open())
    {
        std::cerr << "Error opening files." << std::endl;
    }
    else if (file1.is_open() && file2.is_open())
    {

        std::vector<char> buffer1(std::istreambuf_iterator<char>(file1), {});
        std::vector<char> buffer2(std::istreambuf_iterator<char>(file2), {});

        if (buffer1.size() != buffer2.size())
        {
            std::cout << "Files have different sizes." << std::endl;
        }
        else
        {
            std::cout << "Files are identical." << std::endl;
        }
    }
}

void CopyCommand::execute(const std::vector<Token> &arguments)
{
    std::vector<std::string> sourcePaths;
    std::string destinationPath;

    for (int i = 0; i < arguments.size() - 1; ++i)
    {
        sourcePaths.push_back(arguments[i].value);
    }

    destinationPath = arguments[arguments.size() - 1].value;

    if (sourcePaths.size() == 1 && fs::is_directory(sourcePaths[0]))
    {
        // Copy all files in a directory
        if (destinationPath.back() != fs::path::preferred_separator)
        {
            destinationPath += fs::path::preferred_separator;
        }

        try
        {
            copyDirectory(sourcePaths[0], destinationPath);
        }
        catch (const std::invalid_argument &e)
        {
            std::cerr << "Error encountering while copying the file" << std::endl;
        }
    }
    else if (sourcePaths.size() > 1)
    {
        // Copy multiple files
        if (destinationPath.back() != fs::path::preferred_separator)
        {
            destinationPath += fs::path::preferred_separator;
        }

        try
        {
            copyFiles(sourcePaths, destinationPath);
        }
        catch (const std::invalid_argument &e)
        {
            std::cerr << "Error encountering while copying the file" << std::endl;
        }
    }
    else
    {
        // Copy a single file
        try
        {
            if (fs::is_directory(destinationPath))
            {
                for (const auto &sourcePath : sourcePaths)
                {
                    destinationPath += "\\" + fs::path(sourcePath).filename().string();
                }
            }
            copyFile(sourcePaths[0], destinationPath);
        }
        catch (const std::invalid_argument &e)
        {
            std::cerr << "Error encountering while copying the file" << std::endl;
        }
    }
}

void CopyCommand::copyFile(const std::string &sourcePath, const std::string &destinationPath)
{
    std::ifstream sourceFile(sourcePath, std::ios::binary);
    if (!sourceFile.is_open())
    {
        std::cerr << "Error opening source file: " << sourcePath << std::endl;
    }

    std::ofstream destinationFile(destinationPath, std::ios::binary);
    if (!destinationFile.is_open())
    {
        std::cerr << "Error opening destination file: " << destinationPath << std::endl;
    }

    std::vector<char> buffer(std::istreambuf_iterator<char>(sourceFile), {});

    destinationFile.write(buffer.data(), buffer.size());

    if (!destinationFile.good())
    {
        std::cerr << "Error writing to destination file: " << destinationPath << std::endl;
    }

    std::cout << "File copied successfully." << std::endl;
}

void CopyCommand::copyFiles(const std::vector<std::string> &sourcePaths, const std::string &destinationDir)
{
    for (const auto &sourcePath : sourcePaths)
    {
        std::string destinationPath = destinationDir;

        // If copying to a directory, append the source file name to the destination directory
        if (fs::is_directory(destinationDir))
        {
            destinationPath += fs::path(sourcePath).filename().string();
        }

        copyFile(sourcePath, destinationPath);
    }
}

void CopyCommand::copyDirectory(const std::string &sourceDir, const std::string &destinationDir)
{
    for (const auto &entry : fs::directory_iterator(sourceDir))
    {
        std::string destinationPath = destinationDir + entry.path().filename().string();

        copyFile(entry.path().string(), destinationPath);
    }
}

void TimeCommand::execute(const std::vector<Token> &arguments)
{
    // Get the current time using the system clock
    auto startT = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    // Convert the time to a string
    std::string startTimeString = std::ctime(&startT);

    std::cout << "Started at: " << startTimeString << std::endl;

    while (!keyPressed())
    {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        // Convert the time to a string
        std::string currentTime = std::ctime(&now);

        // Display the current date and time
        displayTime(currentTime);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "\n";

    auto endT = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::string endTimeString = std::ctime(&endT);

    std::cout << "Ended at: " << endTimeString << std::endl;

    // Convert start and end time strings to std::time_t
    std::time_t startTime = convertToTimeT(startTimeString);
    std::time_t endTime = convertToTimeT(endTimeString);

    // Calculate the time elapsed
    double elapsed_seconds = std::difftime(endTime, startTime);

    // Format the elapsed time
    std::string formattedElapsedTime = formatElapsedTime(elapsed_seconds);

    std::cout << "Time elapsed: " << formattedElapsedTime << std::endl;
}

bool TimeCommand::keyPressed()
{
    return _kbhit();
}

void TimeCommand::displayTime(std::string currentTime)
{
    // Remove the newline character from the currentTime string
    currentTime.erase(std::remove(currentTime.begin(), currentTime.end(), '\n'), currentTime.end());

    std::cout << "\rTime: " << currentTime << std::flush;
}

std::time_t TimeCommand::convertToTimeT(const std::string &timeString)
{
    std::tm tm = {};
    std::istringstream ss(timeString);
    ss >> std::get_time(&tm, "%a %b %d %T %Y");
    return std::mktime(&tm);
}

std::string TimeCommand::formatElapsedTime(double elapsedSeconds)
{
    int hours = static_cast<int>(elapsedSeconds / 3600);
    int minutes = static_cast<int>((elapsedSeconds - (hours * 3600)) / 60);
    int seconds = static_cast<int>(elapsedSeconds - (hours * 3600) - (minutes * 60));

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;

    return oss.str();
}

void TimerCommand::execute(const std::vector<Token> &arguments)
{
    int hours, minutes, seconds = 0;
    bool s_init, m_init, h_init = false;

    if (arguments.size() == 0)
    {
        std::cout << "Enter hours: ";
        std::cin >> hours;
        std::cout << "Enter minutes: ";
        std::cin >> minutes;
        std::cout << "Enter seconds: ";
        std::cin >> seconds;
    }
    else if (arguments.size() == 6)
    {
        for (int i = 1; i <= 5; i += 2)
        {
            if (arguments[i - 1].value == "s")
            {
                if (!s_init)
                {
                    seconds = std::stoi(arguments[i].value);
                    s_init = true;
                }
                else
                {
                    std::cout << "Seconds value already initialized, set to 0 to avoid error" << std::endl;
                    seconds = 0;
                }
            }
            else if (arguments[i - 1].value == "m")
            {
                if (!m_init)
                {
                    minutes = std::stoi(arguments[i].value);
                    m_init = true;
                }
                else
                {
                    std::cout << "Minutes value already initialized, set to 0 to avoid error" << std::endl;
                    minutes = 0;
                }
            }
            else if (arguments[i - 1].value == "h")
            {
                if (!h_init)
                {
                    hours = std::stoi(arguments[i].value);
                    h_init = true;
                }
                else
                {
                    std::cout << "Hours value already initialized, set to 0 to avoid error" << std::endl;
                    hours = 0;
                }
            }
            else
            {
                std::cerr << "Error encounter with one of the arguments" << std::endl;
            }
        }
    }
    else
    {
        std::cerr << "Usage: timer [<unit> <time_value> <unit> <time_value> <unit> <time_value>]" << std::endl;
    }

    // Calculate total duration
    auto duration = std::chrono::hours(hours) + std::chrono::minutes(minutes) + std::chrono::seconds(seconds);

    // Start timer
    std::cout << "Timer started!" << std::endl;
    while (duration.count() > 0)
    {
        display_timer(duration);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        duration -= std::chrono::seconds(1);
    }

    std::cout << "\nTimer expired!" << std::endl;
}

void TimerCommand::display_timer(std::chrono::seconds duration)
{
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    duration -= hours;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    duration -= minutes;
    auto seconds = duration;

    std::cout << "\rTimer: ";
    if (hours.count() < 10)
        std::cout << "0";
    std::cout << hours.count() << ":";
    if (minutes.count() < 10)
        std::cout << "0";
    std::cout << minutes.count() << ":";
    if (seconds.count() < 10)
        std::cout << "0";
    std::cout << seconds.count() << std::flush;
}

void LsofCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() == 1)
    {
        try
        {
            int pidBase = detectNumberBase(arguments[0].value);
            DWORD processId = std::stoul(arguments[0].value, nullptr, pidBase);
            listOpenFiles(processId);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error converting string to DWORD: " << e.what() << std::endl;
        }
    }
    else
    {
        std::cerr << "Usage: lsof <PID>" << std::endl;
    }
}

void LsofCommand::listOpenFiles(DWORD processId)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);

    if (hProcess == NULL)
    {
        std::cerr << "Failed to open process with PID " << processId << ". Error code: " << GetLastError() << std::endl;
        return;
    }

    // Get the list of module handles
    HMODULE hModules[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded))
    {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            char szModuleName[MAX_PATH];
            if (GetModuleFileNameExA(hProcess, hModules[i], szModuleName, sizeof(szModuleName) / sizeof(char)))
            {
                std::cout << "File: " << szModuleName << std::endl;
            }
        }
    }
    else
    {
        std::cerr << "Failed to enumerate process modules. Error code: " << GetLastError() << std::endl;
    }

    CloseHandle(hProcess);
}

void MemadrsCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() == 2)
    {
        try
        {
            DWORD processId = std::stoul(arguments[0].value, nullptr);
            std::string outputFilePath = arguments[1].value;

            extractMemoryAddresses(processId, outputFilePath);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error converting string to DWORD: " << e.what() << std::endl;
        }
    }
    else if (arguments.size() == 1)
    {
        try
        {
            DWORD processId = std::stoul(arguments[0].value, nullptr);
            std::string outputFilePath = "./\\memory_address";

            extractMemoryAddresses(processId, outputFilePath);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error converting string to DWORD: " << e.what() << std::endl;
        }
    }
    else
    {
        std::cerr << "Usage: memaddress <PID> <output_file>" << std::endl;
    }
}

void MemadrsCommand::extractMemoryAddresses(DWORD processId, const std::string &outputFilePath)
{
    EnableDebugPrivileges();

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);

    if (hProcess == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to open process with PID " << processId << ". Error code: " << GetLastError() << std::endl;

        DWORD dwError = GetLastError();
        if (dwError == ERROR_INVALID_PARAMETER)
        {
            std::cerr << "Invalid parameter. The process ID may be incorrect or the process does not exist." << std::endl;
        }
        else if (dwError == ERROR_ACCESS_DENIED)
        {
            std::cerr << "Access denied. Ensure that your program has the necessary permissions." << std::endl;
        }

        return;
    }

    MODULEINFO moduleInfo;
    if (GetModuleInformation(hProcess, GetProcessModuleHandle(processId), &moduleInfo, sizeof(moduleInfo)))
    {
        // Code Section
        std::cout << "Code Section: " << std::hex << reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll) << " - "
                  << reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll) + moduleInfo.SizeOfImage << std::endl;

        // Data Section
        PIMAGE_NT_HEADERS pNTHeader = ImageNtHeader(moduleInfo.lpBaseOfDll);
        if (pNTHeader != NULL)
        {
            IMAGE_SECTION_HEADER *pSectionHeader = IMAGE_FIRST_SECTION(pNTHeader);
            for (int i = 0; i < pNTHeader->FileHeader.NumberOfSections; ++i)
            {
                std::cout << "Data Section: " << std::hex
                          << reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll) + pSectionHeader[i].VirtualAddress
                          << " - "
                          << reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll) + pSectionHeader[i].VirtualAddress +
                                 pSectionHeader[i].Misc.VirtualSize
                          << std::endl;
            }
        }

        // Heap
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(hProcess, reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&pmc), sizeof(pmc)))
        {
            std::cout << "Heap: " << std::hex << pmc.PrivateUsage << " - " << pmc.PrivateUsage + pmc.PagefileUsage << std::endl;
        }

        // Stack
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        MEMORY_BASIC_INFORMATION memoryInfo;
        void *stackPointer = nullptr;
        while (VirtualQueryEx(hProcess, stackPointer, &memoryInfo, sizeof(memoryInfo)) == sizeof(memoryInfo))
        {
            if (memoryInfo.Type == MEM_COMMIT && memoryInfo.State == MEM_COMMIT &&
                (memoryInfo.AllocationProtect == PAGE_READWRITE ||
                 memoryInfo.AllocationProtect == PAGE_EXECUTE_READWRITE))
            {
                std::cout << "Stack: " << std::hex << reinterpret_cast<uintptr_t>(memoryInfo.BaseAddress) << " - "
                          << reinterpret_cast<uintptr_t>(memoryInfo.BaseAddress) + memoryInfo.RegionSize << std::endl;
            }
            stackPointer = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(memoryInfo.BaseAddress) + memoryInfo.RegionSize);
        }

        // Save to file
        std::ofstream outputFile(outputFilePath);
        if (outputFile.is_open())
        {
            outputFile << "Code Section: " << std::hex << reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll) << " - "
                       << reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll) + moduleInfo.SizeOfImage << std::endl;
            if (pNTHeader != NULL)
            {
                IMAGE_SECTION_HEADER *pSectionHeader = IMAGE_FIRST_SECTION(pNTHeader);
                for (int i = 0; i < pNTHeader->FileHeader.NumberOfSections; ++i)
                {
                    outputFile << "Data Section: " << std::hex
                               << reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll) + pSectionHeader[i].VirtualAddress
                               << " - "
                               << reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll) + pSectionHeader[i].VirtualAddress +
                                      pSectionHeader[i].Misc.VirtualSize
                               << std::endl;
                }
            }
            outputFile << "Heap: " << std::hex << pmc.PrivateUsage << " - " << pmc.PrivateUsage + pmc.PagefileUsage
                       << std::endl;
            stackPointer = nullptr;
            while (VirtualQueryEx(hProcess, stackPointer, &memoryInfo, sizeof(memoryInfo)) == sizeof(memoryInfo))
            {
                if (memoryInfo.Type == MEM_COMMIT && memoryInfo.State == MEM_COMMIT &&
                    (memoryInfo.AllocationProtect == PAGE_READWRITE ||
                     memoryInfo.AllocationProtect == PAGE_EXECUTE_READWRITE))
                {
                    outputFile << "Stack: " << std::hex
                               << reinterpret_cast<uintptr_t>(memoryInfo.BaseAddress) << " - "
                               << reinterpret_cast<uintptr_t>(memoryInfo.BaseAddress) + memoryInfo.RegionSize
                               << std::endl;
                }
                stackPointer =
                    reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(memoryInfo.BaseAddress) + memoryInfo.RegionSize);
            }
            std::cout << "Memory addresses saved to: " << outputFilePath << std::endl;
        }
        else
        {
            std::cerr << "Failed to open output file: " << outputFilePath << std::endl;
        }
    }
    else
    {
        std::cerr << "Failed to get module information. Error code: " << GetLastError() << std::endl;
    }

    CloseHandle(hProcess);
}

HMODULE MemadrsCommand::GetProcessModuleHandle(DWORD processId)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL)
    {
        DWORD dwError = GetLastError();
        std::cerr << "Failed to open process with PID " << processId << ". Error code: " << dwError << std::endl;

        if (dwError == ERROR_ACCESS_DENIED)
        {
            std::cerr << "Access denied. Ensure that your program has the necessary permissions." << std::endl;
        }
        else if (dwError == ERROR_INVALID_PARAMETER)
        {
            std::cerr << "Invalid parameter. The process ID may be incorrect or the process does not exist." << std::endl;
        }

        return NULL;
    }

    HMODULE hModules[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded))
    {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            TCHAR szModName[MAX_PATH];
            if (GetModuleFileNameEx(hProcess, hModules[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
            {
                CloseHandle(hProcess);
                return hModules[i];
            }
        }
    }
    else
    {
        std::cerr << "Failed to enumerate process modules. Error code: " << GetLastError() << std::endl;
    }

    CloseHandle(hProcess);
    return NULL;
}

void RmaCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() == 3)
    {
        try
        {
            DWORD processId = std::stoul(arguments[0].value, nullptr);

            std::string memoryType = arguments[1].value;

            int memoryBase = detectNumberBase(arguments[2].value);
            std::string stringMemoryAddress = arguments[2].value;
            unsigned long ulMemoryAddress = std::stoul(stringMemoryAddress, nullptr, memoryBase);
            uintptr_t memoryAddress = static_cast<uintptr_t>(ulMemoryAddress);

            readMemoryAddresses(processId, memoryType, memoryAddress);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error converting string to DWORD: " << e.what() << std::endl;
        }
    }
    else
    {
        std::cerr << "Usage: rma <PID> <memory_type> <memory_address>" << std::endl;
    }
}

bool RmaCommand::CheckMemoryProtection(HANDLE hProcess, uintptr_t address)
{
    MEMORY_BASIC_INFORMATION memInfo;
    if (VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(address), &memInfo, sizeof(memInfo)) == sizeof(memInfo))
    {
        return (memInfo.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)) != 0;
    }
    return false;
}

bool RmaCommand::CheckAddressAlignment(uintptr_t address, size_t dataSize)
{
    return (address % sizeof(DWORD) == 0) && (dataSize % sizeof(DWORD) == 0);
}

bool RmaCommand::IsMemoryAddressValid(HANDLE hProcess, uintptr_t address)
{
    MEMORY_BASIC_INFORMATION memInfo;
    return VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(address), &memInfo, sizeof(memInfo)) == sizeof(memInfo);
}

void RmaCommand::VerifyMemoryAccess(HANDLE hProcess, LPVOID address)
{
    MEMORY_BASIC_INFORMATION memoryInfo;

    EnableDebugPrivileges();

    if (VirtualQueryEx(hProcess, address, &memoryInfo, sizeof(memoryInfo)) == sizeof(memoryInfo))
    {
        if (memoryInfo.State == MEM_COMMIT && memoryInfo.Protect != PAGE_NOACCESS)
        {
            std::cout << "Memory at address " << address << " is accessible." << std::endl;
            std::cout << "Base Address: " << memoryInfo.BaseAddress << std::endl;
            std::cout << "Region Size: " << memoryInfo.RegionSize << " bytes" << std::endl;
            std::cout << "Protection: " << memoryInfo.Protect << std::endl;
        }
        else
        {
            std::cerr << "Memory at address " << address << " is not accessible." << std::endl;
        }
    }
    else
    {
        std::cerr << "VirtualQueryEx failed. Error code: " << GetLastError() << std::endl;
    }
}

/*
void RmaCommand::readMemoryAddresses(DWORD processId, std::string &memoryType, uintptr_t &memoryAddress)
{
    // Open the process with appropriate access rights
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL)
    {
        std::cerr << "Failed to open process. Error code: " << GetLastError() << std::endl;
        return;
    }

    // Read the data from the address
    const size_t bufferSize = 512; // Adjust the buffer size as needed
    BYTE buffer[bufferSize];
    SIZE_T bytesRead;

    if (ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(memoryAddress), buffer, sizeof(buffer), &bytesRead))
    {
        // Print the first few bytes from the specified memory region
        std::cout << "Bytes from " << memoryType << " at address " << std::hex << memoryAddress << ": ";
        for (SIZE_T i = 0; i < bytesRead; ++i)
        {
            std::cout << std::hex << static_cast<int>(buffer[i]) << " ";
        }
        std::cout << std::endl;
    }
    else
    {
        DWORD error = GetLastError();
        std::cerr << "Failed to read process memory. Error code: " << error << std::endl;

        if (error == ERROR_PARTIAL_COPY)
        {
            std::cerr << "Not all bytes were read. Bytes read: " << bytesRead << std::endl;
        }
    }

    // Close the process handle
    CloseHandle(hProcess);
}
*/

void RmaCommand::readMemoryAddresses(DWORD processId, std::string &memoryType, uintptr_t &memoryAddress)
{
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);

    if (hProcess == NULL)
    {
        DWORD dwError = GetLastError();
        std::cerr << "Failed to open process with PID " << processId << ". Error code: " << dwError << std::endl;

        if (dwError == ERROR_ACCESS_DENIED)
        {
            std::cerr << "Access denied. Ensure that your program has the necessary permissions." << std::endl;
        }
        else if (dwError == ERROR_INVALID_PARAMETER)
        {
            std::cerr << "Invalid parameter. The process ID may be incorrect or the process does not exist." << std::endl;
        }
    }

    // VerifyMemoryAccess(hProcess, static_cast<LPVOID>(reinterpret_cast<void *>(memoryAddress)));

    // Read the data from the address
    const size_t bufferSize = 1024; // Adjust the buffer size as needed
    BYTE buffer[bufferSize];

    SIZE_T bytesRead;

    /* Memory address test and verification
    // Check Memory Protection
    if (!CheckMemoryProtection(hProcess, memoryAddress))
    {
        std::cerr << "Memory protection check failed. The memory is not readable." << std::endl;
        CloseHandle(hProcess);
    }

    // Check Address Alignment
    if (!CheckAddressAlignment(memoryAddress, bufferSize))
    {
        std::cerr << "Address alignment check failed. Ensure proper alignment." << std::endl;
        CloseHandle(hProcess);
    }

    // Check if the memory address is valid within the target process
    if (!IsMemoryAddressValid(hProcess, memoryAddress))
    {
        std::cerr << "Invalid memory address within the target process." << std::endl;
        CloseHandle(hProcess);
    }
    */

    if (memoryType == "code")
    {
        if (ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(memoryAddress), buffer, sizeof(buffer), &bytesRead))
        {
            // Print the first few bytes from the Code Section
            std::cout << "Bytes from Code Section at address " << std::hex << memoryAddress << ": ";
            for (SIZE_T i = 0; i < bytesRead; ++i)
            {
                std::cout << std::hex << (int)buffer[i] << " ";
            }
            std::cout << std::endl;
        }
        else
        {
            std::cerr << "Failed to read process memory. Error code: " << GetLastError() << std::endl;
        }

        // Close the process handle
        CloseHandle(hProcess);
    }
    else if (memoryType == "data")
    {
        if (ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(memoryAddress), buffer, sizeof(buffer), &bytesRead))
        {
            // Print the first few bytes from the Data Section
            std::cout << "Bytes from Data Section at address " << std::hex << memoryAddress << ": ";
            for (SIZE_T i = 0; i < bytesRead; ++i)
            {
                std::cout << std::hex << static_cast<int>(buffer[i]) << " ";
            }
            std::cout << std::endl;
        }
        else
        {
            DWORD error = GetLastError();
            std::cerr << "Failed to read process memory. Error code: " << error << std::endl;

            if (error == ERROR_PARTIAL_COPY)
            {
                std::cerr << "Not all bytes were read. Bytes read: " << bytesRead << std::endl;
            }
        }

        // Close the process handle
        CloseHandle(hProcess);
    }
    else if (memoryType == "heap")
    {
        if (ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(memoryAddress), buffer, sizeof(buffer), &bytesRead))
        {
            // Print the first few bytes from the Heap
            std::cout << "Bytes from Heap at address " << std::hex << memoryAddress << ": ";
            for (SIZE_T i = 0; i < bytesRead; ++i)
            {
                std::cout << std::hex << static_cast<int>(buffer[i]) << " ";
            }
            std::cout << std::endl;
        }
        else
        {
            std::cerr << "Failed to read process memory. Error code: " << GetLastError() << std::endl;
        }

        // Close the process handle
        CloseHandle(hProcess);
    }
    else if (memoryType == "stacks")
    {
        if (ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(memoryAddress), buffer, sizeof(buffer), &bytesRead))
        {
            // Print the first few bytes from the Stack
            std::cout << "Bytes from Stack at address " << std::hex << memoryAddress << ": ";
            for (SIZE_T i = 0; i < bytesRead; ++i)
            {
                std::cout << std::hex << static_cast<int>(buffer[i]) << " ";
            }
            std::cout << std::endl;
        }
        else
        {
            std::cerr << "Failed to read process memory. Error code: " << GetLastError() << std::endl;
        }

        // Close the process handle
        CloseHandle(hProcess);
    }
    else
    {
        std::cerr << "Unknow process memory type. Error code: " << GetLastError() << std::endl;
    }
}

void HexdumpCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() == 1)
    {
        std::string filePath = arguments[0].value;

        hexdumpC(filePath);
    }
    else if (arguments.size() == 2 || arguments.size() == 3 && arguments[1].value == "-sf")
    {
        if (arguments.size() == 2)
        {
            try
            {
                std::string filePath = arguments[0].value;
                std::string saveFilePath = "./\\hexdump.txt";

                hexdumpF(filePath, saveFilePath);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error encounter with one file path: " << e.what() << std::endl;
            }
        }
        else if (arguments.size() == 3)
        {
            try
            {
                std::string filePath = arguments[0].value;
                std::string saveFilePath = arguments[2].value;

                hexdumpF(filePath, saveFilePath);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error encounter with one file path: " << e.what() << std::endl;
            }
        }
    }
    else
    {
        std::cerr << "Usage: hexdump <file_path> [-sf [<save_file_path>]]" << std::endl;
    }
}

void HexdumpCommand::hexdumpC(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "Error opening file: " << filePath << std::endl;
        return;
    }

    std::cout << "Hexadecimal dump of file: " << filePath << std::endl;

    // Set width for better formatting
    const int width = 16;

    // Read and display the file content in hexadecimal
    char buffer[width];
    while (file.read(buffer, width))
    {
        for (int i = 0; i < width; ++i)
        {
            // Print hex representation with leading zeros
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(buffer[i]) << " ";
        }

        std::cout << "  | ";

        // Print ASCII representation
        for (int i = 0; i < width; ++i)
        {
            char c = buffer[i];
            // Print printable characters, replace others with a dot
            std::cout << (isprint(c) ? c : '.');
        }

        std::cout << std::endl;
    }

    // Handle the remaining bytes
    int remaining_bytes = file.gcount();
    for (int i = 0; i < remaining_bytes; ++i)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(buffer[i]) << " ";
    }

    // Add padding spaces for alignment
    for (int i = 0; i < width - remaining_bytes; ++i)
    {
        std::cout << "   ";
    }

    std::cout << "  | ";

    // Print ASCII representation for the remaining bytes
    for (int i = 0; i < remaining_bytes; ++i)
    {
        char c = buffer[i];
        std::cout << (isprint(c) ? c : '.');
    }

    std::cout << std::endl;

    file.close();
}

void HexdumpCommand::hexdumpF(const std::string &filePath, const std::string &saveFilePath)
{
    std::ifstream file(filePath, std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "Error opening file: " << filePath << std::endl;
        return;
    }

    std::ostream *output_stream;
    std::ofstream save_file;

    save_file.open(saveFilePath, std::ios::out | std::ios::trunc);
    if (!save_file.is_open())
    {
        std::cerr << "Error opening save file: " << saveFilePath << std::endl;
        return;
    }
    output_stream = &save_file;

    (*output_stream) << "Hexadecimal dump of file: " << filePath << std::endl;

    // Set width for better formatting
    const int width = 16;

    // Read and display the file content in hexadecimal
    char buffer[width];
    while (file.read(buffer, width))
    {
        for (int i = 0; i < width; ++i)
        {
            // Print hex representation with leading zeros
            (*output_stream) << std::hex << std::setw(2) << std::setfill('0')
                             << static_cast<int>(buffer[i]) << " ";
        }

        (*output_stream) << "  | ";

        // Print ASCII representation
        for (int i = 0; i < width; ++i)
        {
            char c = buffer[i];
            // Print printable characters, replace others with a dot
            (*output_stream) << (isprint(c) ? c : '.');
        }

        (*output_stream) << std::endl;
    }

    // Handle the remaining bytes
    int remaining_bytes = file.gcount();
    for (int i = 0; i < remaining_bytes; ++i)
    {
        (*output_stream) << std::hex << std::setw(2) << std::setfill('0')
                         << static_cast<int>(buffer[i]) << " ";
    }

    // Add padding spaces for alignment
    for (int i = 0; i < width - remaining_bytes; ++i)
    {
        (*output_stream) << "   ";
    }

    (*output_stream) << "  | ";

    // Print ASCII representation for the remaining bytes
    for (int i = 0; i < remaining_bytes; ++i)
    {
        char c = buffer[i];
        (*output_stream) << (isprint(c) ? c : '.');
    }

    (*output_stream) << std::endl;

    file.close();
    save_file.close(); // Close the save file stream if it was opened
}

void ExtractstrCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() == 1)
    {
        try
        {
            std::string filePath = arguments[0].value;
            std::string saveFilePath = "./\\extracted_str.txt";

            extractStrings(filePath, saveFilePath);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error with the file path: " << e.what() << std::endl;
        }
    }
    else if (arguments.size() == 2)
    {
        try
        {
            std::string filePath = arguments[0].value;
            std::string saveFilePath = arguments[1].value;

            extractStrings(filePath, saveFilePath);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error with the file path: " << e.what() << std::endl;
        }
    }
    else
    {
        std::cerr << "Usage: extractstr <file_path> <save_file_path>" << std::endl;
    }
}

void ExtractstrCommand::extractStrings(const std::string &filePath, const std::string &saveFilePath)
{
    std::vector<std::string> strings;
    std::ifstream file(filePath, std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "Error opening file: " << filePath << std::endl;
    }

    std::ostream *output_stream;
    std::ofstream save_file;

    save_file.open(saveFilePath, std::ios::out | std::ios::trunc);
    if (!save_file.is_open())
    {
        std::cerr << "Error opening save file: " << saveFilePath << std::endl;
        return;
    }
    output_stream = &save_file;

    std::string buffer;
    char ch;

    while (file.get(ch))
    {
        if (isprint(ch) || ch == '\r' || ch == '\n' || ch == '\t')
        {
            buffer += ch;
        }
        else if (!buffer.empty())
        {
            strings.push_back(buffer);
            buffer.clear();
        }
    }

    if (!buffer.empty())
    {
        strings.push_back(buffer);
    }

    file.close();

    *output_stream << "Extracted Strings:" << std::endl;
    for (const auto &extractedStrings : strings)
    {
        *output_stream << extractedStrings << std::endl;
    }
}

void XmlCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments[0].value == "cp")
    {
        if (arguments.size() == 4)
        {
            try
            {
                std::string folderPath = arguments[1].value;
                std::string paramName = arguments[2].value;
                std::string newValue = arguments[3].value;

                processFolder(folderPath, paramName, newValue);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error with one argument: " << e.what() << std::endl;
            }
        }
        else
        {
            std::cerr << "Usage: xml cp <folder_path> <xml_parameter_name> <new_value>" << std::endl;
        }
    }
    else
    {
        std::cerr << "Usage: xml <xml_operation_command>" << std::endl;
    }
}

void XmlCommand::processFolder(const std::string &folderPath, const std::string &paramName, const std::string &newValue)
{
    int totalCount = 0;

    for (const auto &entry : fs::recursive_directory_iterator(folderPath))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".xml")
        {
            totalCount += changeParameterInXML(entry.path().string(), paramName, newValue);
        }
    }

    std::cout << "Total occurrences modified: " << totalCount << std::endl;
}

int XmlCommand::changeParameterInXML(const std::string &xmlFile, const std::string &paramName, const std::string &newValue)
{
    try
    {
        std::ifstream inputFile(xmlFile);
        if (!inputFile.is_open())
        {
            std::cerr << "Error opening file: " << xmlFile << std::endl;
            return 0;
        }

        std::string content((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());

        // Use regular expression to find and replace the parameter
        std::regex pattern(paramName + "\\s*=\\s*\"[^\"]*\"");
        std::string updatedContent = std::regex_replace(content, pattern, paramName + "=\"" + newValue + "\"");

        auto begin = std::sregex_iterator(content.begin(), content.end(), pattern);
        auto end = std::sregex_iterator();

        int count = std::distance(begin, end);

        inputFile.close();

        std::ofstream outputFile(xmlFile);
        if (!outputFile.is_open())
        {
            std::cerr << "Error opening file for writing: " << xmlFile << std::endl;
            return 0;
        }

        outputFile << updatedContent;
        outputFile.close();

        return count;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error processing " << xmlFile << ": " << e.what() << std::endl;
        return 0;
    }
}

void EncodingCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments[0].value == "xor")
    {
        if (arguments[1].value == "encrypt")
        {
            try
            {
                std::string input_file = arguments[2].value;
                std::string output_file = arguments[3].value;
                std::string encryption_key = arguments[4].value;

                xor_encrypt(input_file, output_file, encryption_key);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error with one argument: " << e.what() << std::endl;
            }
        }
        else if (arguments[1].value == "decrypt")
        {
            try
            {
                std::string input_file = arguments[2].value;
                std::string output_file = arguments[3].value;
                std::string decryption_key = arguments[4].value;

                xor_decrypt(input_file, output_file, decryption_key);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error with one argument: " << e.what() << std::endl;
            }
        }
        else
        {
            std::cerr << "Usage: encoding xor <xor_command> <input_file> <output_file> <encryption/decryption_key>" << std::endl;
        }
    }
    else
    {
        std::cerr << "Usage: encoding <encryption_operation/method>" << std::endl;
    }
}

void EncodingCommand::xor_encrypt(const std::string &input_file, const std::string &output_file, const std::string &encryption_key)
{
    try
    {
        std::ifstream input_stream(input_file, std::ios::binary);
        std::ofstream output_stream(output_file, std::ios::binary);

        if (!input_stream.is_open() || !output_stream.is_open())
        {
            throw std::runtime_error("Error opening files.");
        }

        char byte;
        size_t key_length = encryption_key.length();
        size_t i = 0;

        while (input_stream.get(byte))
        {
            byte ^= encryption_key[i % key_length];
            output_stream.put(byte);
            i++;
        }

        std::cout << "File encrypted successfully. Encrypted file saved at: " << output_file << std::endl;

        input_stream.close();
        output_stream.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error encrypting file: " << e.what() << std::endl;
    }
}

void EncodingCommand::xor_decrypt(const std::string &input_file, const std::string &output_file, const std::string &decryption_key)
{
    try
    {
        std::ifstream input_stream(input_file, std::ios::binary);
        std::ofstream output_stream(output_file, std::ios::binary);

        if (!input_stream.is_open() || !output_stream.is_open())
        {
            throw std::runtime_error("Error opening files.");
        }

        char byte;
        size_t key_length = decryption_key.length();
        size_t i = 0;

        while (input_stream.get(byte))
        {
            byte ^= decryption_key[i % key_length];
            output_stream.put(byte);
            i++;
        }

        std::cout << "File decrypted successfully. Decrypted file saved at: " << output_file << std::endl;

        input_stream.close();
        output_stream.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error decrypting file: " << e.what() << std::endl;
    }
}

void PasswordCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() == 1)
    {
        int length = std::stoi(arguments[0].value);

        try
        {
            std::string password = generatePassword(length);

            if (!password.empty())
            {
                std::cout << "Generated Password: " << password << std::endl;
            }
            else
            {
                std::cout << "Error encounter during password generation" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error decrypting file: " << e.what() << std::endl;
        }
    }
    else
    {
        std::cerr << "Usage: gpw <password_length>" << std::endl;
    }
}

std::string PasswordCommand::generatePassword(int length)
{
    const std::string charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-_=+[]{}|;:'\",.<>?/";
    const int charsetLength = charset.length();

    if (length <= 0)
    {
        std::cerr << "Password length must be greater than 0." << std::endl;
        return "";
    }

    std::string password;
    srand(static_cast<unsigned int>(time(nullptr)));

    for (int i = 0; i < length; ++i)
    {
        int randomIndex = rand() % charsetLength;
        password += charset[randomIndex];
    }

    return password;
}

void QuicksearchCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() == 2)
    {
        if (startsWithPeriod(stringToWstring(arguments[1].value)))
        {
            try
            {
                std::string searchDirectory = arguments[0].value; // You can set the root directory accordingly
                std::string fileExtension = arguments[1].value;

                std::wstring searchDirectoryW = stringToWstring(searchDirectory);
                std::wstring fileExtensionW = stringToWstring(fileExtension);

                std::vector<std::wstring> filePaths;
                filePaths = searchFilesWithExtension(searchDirectoryW, fileExtensionW, filePaths);

                for (const auto &path : filePaths)
                {
                    std::wcout << path << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
        }
        else
        {
            try
            {
                std::string searchDirectory = arguments[0].value; // You can set the root directory accordingly
                std::string fileName = arguments[1].value;

                std::wstring searchDirectoryW = stringToWstring(searchDirectory);
                std::wstring fileNameW = stringToWstring(fileName);

                std::vector<std::wstring> filePaths;
                filePaths = searchFile(searchDirectoryW, fileNameW, filePaths);

                for (const auto &path : filePaths)
                {
                    std::wcout << path << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
        }
    }
    else
    {
        std::cerr << "Usage: qs <search_directory (Ex : 'C:\\')> <file_name>" << std::endl;
    }
}

// Recursive function (Slower)
/*
std::vector<std::wstring> QuicksearchCommand::searchFile(const std::wstring &directory, const std::wstring &fileName, std::vector<std::wstring> &filePaths)
{
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW((directory + L"\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Invalid Handle Windows Error" << std::endl;
    }

    do
    {
        if (wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0)
        {
            std::wstring filePath = directory + L"\\" + charToWchar(findFileData.cFileName);

            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // Recursive call for directories
                searchFile(filePath, fileName, filePaths);
            }
            else if (wcscmp(findFileData.cFileName, fileName.c_str()) == 0)
            {
                // Found the file
                filePaths.push_back(filePath);
            }
        }
    } while (FindNextFileW(hFind, &findFileData) != 0);

    FindClose(hFind);

    return filePaths;
}
*/

// Iterative function using stack
std::vector<std::wstring> QuicksearchCommand::searchFile(const std::wstring &directory, const std::wstring &fileName, std::vector<std::wstring> &filePaths)
{
    std::stack<std::wstring> directories;
    directories.push(directory);

    while (!directories.empty())
    {
        std::wstring currentDir = directories.top();
        directories.pop();

        WIN32_FIND_DATAW findFileData;
        HANDLE hFind = FindFirstFileW((currentDir + L"\\*").c_str(), &findFileData);

        if (hFind == INVALID_HANDLE_VALUE)
        {
            std::cerr << "Invalid Handle Windows Error" << std::endl;
            continue;
        }

        do
        {
            if (wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0)
            {
                std::wstring filePath = currentDir + L"\\" + findFileData.cFileName;

                if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    // Push directories onto the stack for later processing
                    directories.push(filePath);
                }
                else if (wcscmp(findFileData.cFileName, fileName.c_str()) == 0)
                {
                    // Found the file
                    filePaths.push_back(filePath);
                }
            }
        } while (FindNextFileW(hFind, &findFileData) != 0);

        FindClose(hFind);
    }

    return filePaths;
}

std::vector<std::wstring> QuicksearchCommand::searchFilesWithExtension(const std::wstring &directory, const std::wstring &extension, std::vector<std::wstring> &filePaths)
{
    for (const auto &entry : fs::recursive_directory_iterator(directory))
    {
        if (entry.is_regular_file())
        {
            std::wstring fileName = entry.path().filename();
            if (fileName.size() > extension.size() && fileName.substr(fileName.size() - extension.size()) == extension)
            {
                filePaths.push_back(entry.path().wstring());
            }
        }
    }
    return filePaths;
}

void MemstatsCommand::execute(const std::vector<Token> &arguments)
{
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);

    std::cout << "Memory Statistics:" << std::endl;
    std::cout << "------------------" << std::endl;
    std::cout << "Total Physical Memory: " << formatMemory(memStatus.ullTotalPhys) << std::endl;
    std::cout << "Available Physical Memory: " << formatMemory(memStatus.ullAvailPhys) << std::endl;
    std::cout << "Memory Load: " << memStatus.dwMemoryLoad << "%" << std::endl;
}

std::string MemstatsCommand::formatMemory(ULONGLONG bytes)
{
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;

    std::ostringstream oss;
    if (bytes >= GB)
    {
        oss << std::fixed << std::setprecision(2) << static_cast<double>(bytes) / GB << " GB";
    }
    else if (bytes >= MB)
    {
        oss << std::fixed << std::setprecision(2) << static_cast<double>(bytes) / MB << " MB";
    }
    else if (bytes >= KB)
    {
        oss << std::fixed << std::setprecision(2) << static_cast<double>(bytes) / KB << " KB";
    }
    else
    {
        oss << bytes << " bytes";
    }
    return oss.str();
}

void CalculatorCommand::execute(const std::vector<Token> &arguments)
{
    std::string input;

    std::cout << "Calculator Mode (Basic Version)\n";

    while (true)
    {
        std::cout << "> ";
        getline(std::cin, input);

        if (input == "exit")
            break;

        double result = calculate(input);
        if (!__isnan(result))
            std::cout << "Result: " << result << std::endl;
    }

    std::cout << "Exiting Calculator Mode\n";
}

double CalculatorCommand::calculate(std::string expression)
{
    std::istringstream iss(expression);
    double result = 0.0;
    char op;
    iss >> result; // Read the first operand

    // Read operator and second operand, and perform calculations
    while (iss >> op)
    {
        double operand;
        iss >> operand;
        switch (op)
        {
        case '+':
            result += operand;
            break;
        case '-':
            result -= operand;
            break;
        case '*':
            result *= operand;
            break;
        case '/':
            if (operand != 0)
                result /= operand;
            else
            {
                std::cout << "Error: Division by zero!" << std::endl;
                return NAN; // Not a Number
            }
            break;
        default:
            std::cout << "Error: Invalid operator " << op << std::endl;
            return NAN;
        }
    }
    return result;
}

void KillCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() == 1)
    {
        DWORD pid = std::stoul(arguments[0].value);

        // Open the process
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (hProcess == NULL)
        {
            std::cerr << "Failed to open process with PID " << pid << std::endl;
        }

        // Terminate the process
        if (!TerminateProcess(hProcess, 0))
        {
            std::cerr << "Failed to terminate process with PID " << pid << std::endl;
            CloseHandle(hProcess);
        }

        std::cout << "Process with PID " << pid << " terminated successfully." << std::endl;

        // Close the process handle
        CloseHandle(hProcess);
    }
    else
    {
        std::cerr << "Usage: kill <PID>" << std::endl;
    }
}

void RandomCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() >= 1)
    {
        // Seed the random number generator with current time
        srand(time(nullptr));

        if (arguments[0].value == "number")
        {
            try
            {
                int length = std::stoi(arguments[1].value);

                if (length <= 0)
                {
                    std::cerr << "Invalid length. Length should be a positive integer." << std::endl;
                }

                std::string randomNumber = generateRandomNumber(length);
                std::cout << "Random number of length " << length << ": " << randomNumber << std::endl;
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
                std::cerr << "Usage: random number <length>" << std::endl;
            }
        }
        else if (arguments[0].value == "coin")
        {
            try
            {
                std::cout << "Press any key to toss the coin..." << std::endl;
                std::cin.get(); // Wait for user to press any key

                std::string outcome = tossCoin();
                std::cout << "The coin landed on: " << outcome << std::endl;
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
        }
    }
    else
    {
        std::cerr << "Usage: random <operation>" << std::endl;
    }
}

std::string RandomCommand::generateRandomNumber(int length)
{
    std::string randomNumber;
    for (int i = 0; i < length; ++i)
    {
        if (i == 0)
        {
            // Ensure the most significant digit is not 0
            randomNumber += '1' + rand() % 9;
        }
        else
        {
            randomNumber += '0' + rand() % 10;
        }
    }
    return randomNumber;
}

std::string RandomCommand::tossCoin()
{
    // Generate a random number (0 or 1) representing heads or tails
    int result = rand() % 2;

    // Return "heads" if result is 0, otherwise return "tails"
    return (result == 0) ? "heads" : "tails";
}

void EnvvarCommand::execute(const std::vector<Token> &arguments)
{
    std::string name;
    std::string value;

    if (arguments.size() >= 1)
    {
        if (arguments[0].value == "set")
        {
            if (arguments.size() == 3)
            {
                name = arguments[1].value;
                value = arguments[2].value;

                if (containsSpace(value))
                {
                    setEnvVar(name, "\"" + value + "\"");
                }
                else
                {
                    setEnvVar(name, value);
                }
            }
            else
            {
                std::cout << "Enter variable name: ";
                std::cin >> name;
                std::cout << "Enter variable value: ";
                std::cin >> value;
                if (containsSpace(value))
                {
                    setEnvVar(name, "\"" + value + "\"");
                }
                else
                {
                    setEnvVar(name, value);
                }
            }
        }
        else if (arguments[0].value == "get")
        {
            if (arguments.size() == 2)
            {
                name = arguments[1].value;

                getEnvVar(name);
            }
            else
            {
                std::cout << "Enter variable name: ";
                std::cin >> name;
                getEnvVar(name);
            }
        }
        else if (arguments[0].value == "unset")
        {
            if (arguments.size() == 2)
            {
                name = arguments[1].value;

                unsetEnvVar(name);
            }
            else
            {
                std::cout << "Enter variable name: ";
                std::cin >> name;
                unsetEnvVar(name);
            }
        }
        else
        {
            std::cerr << "Usage: envvar <operation (set/unset/get)>" << std::endl;
        }
    }
    else
    {
        std::cerr << "Usage: envvar <operation (set/unset/get)>" << std::endl;
    }
}

// Function to set environment variable
void EnvvarCommand::setEnvVar(const std::string &name, const std::string &value)
{
    if (getenv(name.c_str()) != nullptr)
    {
        std::cerr << "Environment variable " << name << " already exists" << std::endl;
    }
    else
    {
        if (putenv((name + "=" + value).c_str()) != 0)
        {
            std::cerr << "Error setting environment variable: " << name << std::endl;
        }
        else
        {
            std::cout << "Environment variable " << name << " set to: " << value << std::endl;
        }
    }
}

// Function to get environment variable
void EnvvarCommand::getEnvVar(const std::string &name)
{
    char *value = getenv(name.c_str());
    if (value != nullptr)
    {
        std::cout << "Value of environment variable " << name << ": " << value << std::endl;
    }
    else
    {
        std::cerr << "Environment variable " << name << " not found" << std::endl;
    }
}

// Function to unset environment variable
void EnvvarCommand::unsetEnvVar(const std::string &name)
{
    if (getenv(name.c_str()) == nullptr)
    {
        std::cerr << "Environment variable " << name << " doesn't exists" << std::endl;
    }
    else
    {
        if (putenv((name + "=").c_str()) != 0)
        {
            std::cerr << "Error unsetting environment variable: " << name << std::endl;
        }
        else
        {
            std::cout << "Environment variable " << name << " unset successfully" << std::endl;
        }
    }
}

void RemCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() == 2)
    {
        std::string filePath = arguments[0].value;
        std::string pattern = arguments[1].value;

        std::ifstream file(filePath);
        if (!file)
        {
            std::cerr << "Error opening file!" << std::endl;
        }

        std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

        int matchCount = countRegexMatches(text, pattern);
        std::cout << "Number of matches: " << matchCount << std::endl;
    }
    else
    {
        std::cerr << "Usage: rem <file_path> <regular_expression>" << std::endl;
    }
}

int RemCommand::countRegexMatches(const std::string &text, const std::string &pattern)
{
    std::regex regexPattern(pattern);
    std::sregex_iterator iter(text.begin(), text.end(), regexPattern);
    std::sregex_iterator end;

    int count = 0;
    while (iter != end)
    {
        ++count;
        ++iter;
    }

    return count;
}

void SchemaCommand::execute(const std::vector<Token> &arguments)
{
    if (arguments.size() >= 1)
    {
        if (arguments[0].value == "dependency")
        {
            if (arguments.size() == 2)
            {
                std::string entryFile = arguments[1].value;

                dependencyTreeCommand(entryFile);
            }
            else
            {
                std::cerr << "Usage: schema dependency <entry_file_path>" << std::endl;
            }
        }
        else if (arguments[0].value == "folder")
        {
            if (arguments.size() == 2)
            {
                fs::path folderPath = arguments[1].value;

                if (!fs::exists(folderPath))
                {
                    std::cerr << "Path does not exist.\n";
                }

                if (!fs::is_directory(folderPath))
                {
                    std::cerr << "Provided path is not a folder.\n";
                }

                size_t currentEntryIndex = 0;

                std::cout << folderPath << "\n";
                generateFolderTree(folderPath, 0, false, 0, currentEntryIndex, false, 0);
            }
            else
            {
                std::cerr << "Usage: schema folder <folder_path>" << std::endl;
            }
        }
    }
    else
    {
        std::cerr << "Usage: schema <schema_type>" << std::endl;
    }
}

void SchemaCommand::dependencyTreeCommand(const std::string &entryFile)
{
    parseFile(entryFile);
    std::cout << "Dependency Tree:" << std::endl;
    std::unordered_set<std::string> visited;
    generateDependencyTree(entryFile, 0, visited);
}

// Function to parse a file and extract its dependencies
void SchemaCommand::parseFile(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error: Failed to open file " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        // Regular expression to match #include directives
        std::regex includeRegex("#include[\\s]+[\"<]([^\">]+)[\">]");
        std::smatch match;

        if (std::regex_search(line, match, includeRegex))
        {
            if (match.size() == 2)
            {
                dependencyGraph[filename].insert(match[1].str());
            }
        }
    }

    file.close();
}

// Function to recursively generate the dependency tree
void SchemaCommand::generateDependencyTree(const std::string &file, int depth, std::unordered_set<std::string> &visited)
{
    if (visited.count(file) > 0) // Limiting depth and preventing cyclic dependencies
        return;

    visited.insert(file);

    // Print indentation based on depth
    for (int i = 0; i < depth; ++i)
        std::cout << "  ";

    std::cout << "|-- " << file << std::endl;

    // Recursively call for dependencies
    for (const std::string &dependency : dependencyGraph[file])
    {
        generateDependencyTree(dependency, depth + 1, visited);
    }
}

void SchemaCommand::generateFolderTree(const fs::path &folderPath, int level, bool isLast, size_t totalEntries, size_t currentEntryIndex, bool parentIsLast, size_t lastFolderCount)
{
    if (fs::is_directory(folderPath))
    {

        // Get the number of entries in the current directory
        size_t totalEntries = 0;
        for (const auto &entryTemp : fs::directory_iterator(folderPath))
        {
            ++totalEntries;
        }

        size_t currentIndex = 0;
        for (const auto &entry : fs::directory_iterator(folderPath))
        {
            ++currentEntryIndex;
            ++currentIndex;

            bool isLastEntry = currentEntryIndex == totalEntries;

            std::string indent = "";

            if (parentIsLast)
            {
                for (int i = 0; i < level - (4 * lastFolderCount); ++i)
                {
                    if (i % 4 == 0)
                    {
                        indent += "│   ";
                    }
                    else
                    {
                        continue;
                    }
                }

                for (size_t it = 0; it < lastFolderCount; ++it)
                {
                    indent += "    ";
                }
            }
            else
            {
                for (int i = 0; i < level; ++i)
                {
                    if (i % 4 == 0)
                    {
                        indent += "│   ";
                    }
                    else
                    {
                        continue;
                    }
                }
            }

            std::string entryStr = entry.path().filename().string();
            if (entry.is_directory())
            {
                std::cout << indent;
                if (currentIndex < totalEntries)
                {
                    std::cout << "├── ";
                    parentIsLast = false;
                }
                else
                {
                    std::cout << "└── ";
                    parentIsLast = true;
                    lastFolderCount += 1;
                }
                std::cout << entryStr << " " << lastFolderCount << "/\n";
                generateFolderTree(entry.path(), level + 4, isLastEntry, totalEntries, currentEntryIndex, parentIsLast, lastFolderCount);
            }
            else
            {
                std::cout << indent;
                if (currentIndex < totalEntries)
                {
                    std::cout << "├── ";
                }
                else
                {
                    std::cout << "└── ";
                    if (parentIsLast)
                    {
                        lastFolderCount -= 1;
                    }
                }
                std::cout << entryStr << " " << lastFolderCount << "\n";
            }
        }
    }
}
