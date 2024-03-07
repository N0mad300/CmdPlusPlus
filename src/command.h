#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>
#include <unordered_set>
#include <filesystem>

#include "tokenizer.h"
#include "utils.h"

using namespace Tokenizer;
using namespace Utils;
using namespace fs;

namespace Exe
{
    void executeCommand(const std::wstring &command);
}

class Command
{
public:
    virtual void execute(const std::vector<Token> &arguments) = 0;
    // Add other common functions or data members if needed
    virtual ~Command() {}
};

class EchoCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;
};

class CdCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    void changeDirectory(const std::string &newDir);
};

class AssocCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;
};

class ClsCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    void clearScreen();
};

class CmdCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
#ifdef _WIN32
    void startNewProcess();
#else
    void forkAndExec();
#endif
};

class ColorCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    void changeTextColor(const std::string &color);
    int parseHexColor(const std::string &hexColor);
};

class CompCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    void compFile(const std::string &filePath1, const std::string &filePath2);
};

class CopyCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    void copyFile(const std::string &sourcePath, const std::string &destinationPath);
    void copyFiles(const std::vector<std::string> &sourcePaths, const std::string &destinationDir);
    void copyDirectory(const std::string &sourceDir, const std::string &destinationDir);
};

class TimeCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    bool keyPressed();
    void displayTime(std::string currentTime);
    std::time_t convertToTimeT(const std::string &timeString);
    std::string formatElapsedTime(double elapsedSeconds);
};

class TimerCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    void display_timer(std::chrono::seconds duration);
};

class LsofCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    void listOpenFiles(DWORD processId);
};

class MemadrsCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    void extractMemoryAddresses(DWORD processId, const std::string &outputFilePath);
    HMODULE GetProcessModuleHandle(DWORD processId);
};

class RmaCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    void readMemoryAddresses(DWORD processId, std::string &memoryType, uintptr_t &memoryAddress);
    void VerifyMemoryAccess(HANDLE hProcess, LPVOID address);

    bool CheckMemoryProtection(HANDLE hProcess, uintptr_t address);
    bool CheckAddressAlignment(uintptr_t address, size_t dataSize);
    bool IsMemoryAddressValid(HANDLE hProcess, uintptr_t address);
};

class HexdumpCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    void hexdumpC(const std::string &filePath);                                  // CLI version : Display the hexadecimal representation in the console
    void hexdumpF(const std::string &filePath, const std::string &saveFilePath); // File version : Save the hexadecimal representation in a file
};

class ExtractstrCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    void extractStrings(const std::string &filePath, const std::string &saveFilePath);
};

class XmlCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    void processFolder(const std::string &folderPath, const std::string &paramName, const std::string &newValue);

    int changeParameterInXML(const std::string &xmlFile, const std::string &paramName, const std::string &newValue);
};

class EncodingCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    void xor_encrypt(const std::string &input_file, const std::string &output_file, const std::string &encryption_key);
    void xor_decrypt(const std::string &input_file, const std::string &output_file, const std::string &decryption_key);
};

class PasswordCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    std::string generatePassword(int length);
};

class QuicksearchCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    std::vector<std::wstring> searchFile(const std::wstring &directory, const std::wstring &fileName, std::vector<std::wstring> &filePaths);
    std::vector<std::wstring> searchFilesWithExtension(const std::wstring &directory, const std::wstring &extension, std::vector<std::wstring> &filePaths);
};

class MemstatsCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    std::string formatMemory(ULONGLONG bytes);
};

class CalculatorCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    double calculate(std::string expression);
};

class KillCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;
};

class RandomCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    std::string generateRandomNumber(int length);
    std::string tossCoin();
};

class EnvvarCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    void setEnvVar(const std::string &name, const std::string &value);
    void unsetEnvVar(const std::string &name);
    void getEnvVar(const std::string &name);
};

class RemCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    int countRegexMatches(const std::string &text, const std::string &pattern);
};

class SchemaCommand : public Command
{
public:
    void execute(const std::vector<Token> &arguments) override;

private:
    // Method for the folder schema
    size_t functionCallCount;
    void generateFolderTree(const fs::path &folderPath, int level, bool isLast, size_t totalEntries, size_t currentEntryIndex, bool parentIsLast, size_t lastFolderCount);

    // Method for the dependency schema
    std::unordered_map<std::string, std::unordered_set<std::string>> dependencyGraph;

    void parseFile(const std::string &filename);
    void dependencyTreeCommand(const std::string &entryFile);
    void generateDependencyTree(const std::string &file, int depth, std::unordered_set<std::string> &visited);
};