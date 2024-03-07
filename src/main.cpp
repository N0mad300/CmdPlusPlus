#include <iostream>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <regex>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <Windows.h>

#include "command.h"
#include "tokenizer.h"
#include "utils.h"

using namespace Tokenizer;
using namespace Utils;
using namespace Exe;
using namespace fs;

class CommandRegistry
{
public:
    void registerCommand(const std::string &name, std::unique_ptr<Command> command)
    {
        commands[name] = std::move(command);
    }

    Command *getCommand(const std::string &name) const
    {
        auto it = commands.find(name);
        return it != commands.end() ? it->second.get() : nullptr;
    }

private:
    std::unordered_map<std::string, std::unique_ptr<Command>> commands;
};

void showPrompt()
{
    std::string currentDir = getCurrentDir();
    std::cout << currentDir << "> ";
}

bool fileExists(const std::wstring &filePath)
{
    DWORD attributes = GetFileAttributesW(filePath.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES) && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool hasExeExtension(const std::wstring &filePath)
{
    size_t dotPosition = filePath.find_last_of(L'.');
    if (dotPosition != std::wstring::npos)
    {
        std::wstring extension = filePath.substr(dotPosition + 1);
        return extension == L"exe";
    }
    return false;
}

bool isExecutable(const std::wstring &filePath)
{
    return fileExists(filePath) && hasExeExtension(filePath);
}

void processSectionCommand(std::string li, const CommandRegistry &commandRegistry, std::map<std::string, std::vector<std::string>> sections)
{
    try
    {
        std::vector<Token> tokens = tokenize(li);
        std::string commandName = tokens[0].value;
        tokens.erase(tokens.begin());

        Command *command = commandRegistry.getCommand(commandName);
        if (command)
        {
            command->execute(tokens);
        }
        else if (commandName == "goto")
        {
            std::string sectionName = tokens[0].value;

            auto section = sections.find(sectionName);

            if (section != sections.end())
            {
                // Access the pair using the iterator
                const std::pair<const std::string, std::vector<std::string>> &specificPair = *section;
                const std::vector<std::string> &commands = specificPair.second;

                for (const std::string &li : commands)
                {
                    processSectionCommand(li, commandRegistry, sections);
                }
            }
            else
            {
                std::cout << "Section not found in the file" << std::endl;
            }
        }
        else
        {
            std::cerr << "Unknown command: " << commandName << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

void executeCommandFile(const std::string &commandFilePath, const CommandRegistry &commandRegistry)
{
    std::ifstream commandFile(commandFilePath);

    if (!commandFile.is_open())
    {
        std::cerr << "Unable to open the file." << std::endl;
    }

    bool inSection = false;

    std::smatch match;

    std::map<std::string, std::vector<std::string>> sections;

    // Define the regular expression pattern
    std::regex sectionStartRegex("/-(\\w+)");
    std::regex sectionEndRegex("(\\w+)-/");

    std::vector<std::string> sectionLines;
    std::vector<std::string> commandLines;

    std::string line;
    std::string currentSection;

    while (std::getline(commandFile, line))
    {
        // Check if the line indicates the start of a section
        if (std::regex_match(line, match, sectionStartRegex))
        {
            currentSection = match[1].str();
            inSection = true;
            continue; // Skip the line indicating the start of the section
        }

        // Check if the line indicates the end of a section
        if (std::regex_match(line, match, sectionEndRegex) && match[1].str() == currentSection)
        {
            sections[currentSection] = sectionLines;
            inSection = false;

            sectionLines.clear();
            currentSection.clear();
            continue; // Skip the line indicating the end of the section
        }

        // If inside a section, store the line
        if (!currentSection.empty() && inSection == true)
        {
            sectionLines.push_back(line);
        }

        // Process each line as needed
        if (inSection == false)
        {
            commandLines.push_back(line);
        }
    }

    commandFile.close();

    for (const auto &l : commandLines)
    {
        try
        {
            std::vector<Token> tokens = tokenize(l);
            std::string commandName = tokens[0].value;
            tokens.erase(tokens.begin());

            Command *command = commandRegistry.getCommand(commandName);
            if (command)
            {
                command->execute(tokens);
            }
            else if (commandName == "goto")
            {
                std::string sectionName = tokens[0].value;

                auto section = sections.find(sectionName);

                if (section != sections.end())
                {
                    // Access the pair using the iterator
                    const std::pair<const std::string, std::vector<std::string>> &specificPair = *section;
                    const std::vector<std::string> &commands = specificPair.second;

                    for (const std::string &li : commands)
                    {
                        processSectionCommand(li, commandRegistry, sections);
                    }
                }
                else
                {
                    std::cout << "Section not found in the file" << std::endl;
                }
            }
            else
            {
                std::cerr << "Unknown command: " << commandName << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    }
}

void processCommand(std::string &cmdline, const CommandRegistry &commandRegistry, bool &exetime, std::vector<Token> tokens)
{
    try
    {
        std::string commandName = cmdline;
        std::wstring wCommandName = stringToWstring(commandName);

        Command *command = commandRegistry.getCommand(commandName);
        if (command)
        {
            if (exetime)
            {
                auto start = std::chrono::high_resolution_clock::now();

                command->execute(tokens);

                auto end = std::chrono::high_resolution_clock::now();

                // Execution time in microseconds
                auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

                // Convert duration to milliseconds
                auto duration_ms = duration_us / 1000.0;

                // Convert duration to seconds
                auto duration_s = duration_us / 1000000.0;

                // Convert duration to minutes
                auto duration_min = duration_us / 60000000.0;

                std::cout << "Execution time: " << std::endl;
                std::cout << duration_us << " microseconds" << std::endl;
                std::cout << duration_ms << " milliseconds" << std::endl;
                std::cout << duration_s << " seconds" << std::endl;
                std::cout << duration_min << " minutes" << std::endl;
            }
            else
            {
                command->execute(tokens);
            }
        }
        else if (isExecutable(wCommandName))
        {
            executeCommand(wCommandName);
        }
        else if (std::filesystem::exists(commandName))
        {
            if (std::filesystem::is_regular_file(commandName))
            {
                std::wstring extension = std::filesystem::path(commandName).extension();
                if (extension == L".shl")
                {
                    executeCommandFile(commandName, commandRegistry);
                }
                else
                {
                    std::cerr << "Unknown command: " << commandName << std::endl;
                }
            }
            else
            {
                std::cerr << "Unknown command: " << commandName << std::endl;
            }
        }
        else if (commandName == "exetime")
        {
            exetime = not exetime;
        }
        else if (std::getenv(commandName.c_str()) != nullptr)
        {
        }
        else
        {
            std::cerr << "Unknown command: " << commandName << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

int main()
{
    CommandRegistry commandRegistry;

    // Register commands
    commandRegistry.registerCommand("echo", std::make_unique<EchoCommand>());
    commandRegistry.registerCommand("cd", std::make_unique<CdCommand>());
    commandRegistry.registerCommand("assoc", std::make_unique<AssocCommand>());
    commandRegistry.registerCommand("cls", std::make_unique<ClsCommand>());
    commandRegistry.registerCommand("cmd", std::make_unique<CmdCommand>());
    commandRegistry.registerCommand("cmd++", std::make_unique<CmdCommand>());
    commandRegistry.registerCommand("color", std::make_unique<ColorCommand>());
    commandRegistry.registerCommand("comp", std::make_unique<CompCommand>());
    commandRegistry.registerCommand("fc", std::make_unique<CompCommand>());
    commandRegistry.registerCommand("copy", std::make_unique<CopyCommand>());
    commandRegistry.registerCommand("time", std::make_unique<TimeCommand>());
    commandRegistry.registerCommand("timer", std::make_unique<TimerCommand>());
    commandRegistry.registerCommand("lsof", std::make_unique<LsofCommand>());
    commandRegistry.registerCommand("memadrs", std::make_unique<MemadrsCommand>());
    commandRegistry.registerCommand("memstats", std::make_unique<MemstatsCommand>());
    commandRegistry.registerCommand("rma", std::make_unique<RmaCommand>()); // WIP
    commandRegistry.registerCommand("hexdump", std::make_unique<HexdumpCommand>());
    commandRegistry.registerCommand("findstr", std::make_unique<ExtractstrCommand>());
    commandRegistry.registerCommand("xml", std::make_unique<XmlCommand>());
    commandRegistry.registerCommand("encoding", std::make_unique<EncodingCommand>());
    commandRegistry.registerCommand("gpw", std::make_unique<PasswordCommand>());
    commandRegistry.registerCommand("quicksearch", std::make_unique<QuicksearchCommand>());
    commandRegistry.registerCommand("qs", std::make_unique<QuicksearchCommand>());
    commandRegistry.registerCommand("clc", std::make_unique<CalculatorCommand>());
    commandRegistry.registerCommand("kill", std::make_unique<KillCommand>());
    commandRegistry.registerCommand("random", std::make_unique<RandomCommand>());
    commandRegistry.registerCommand("envvar", std::make_unique<EnvvarCommand>());
    commandRegistry.registerCommand("rem", std::make_unique<RemCommand>());
    commandRegistry.registerCommand("schema", std::make_unique<SchemaCommand>());

    // Use the custom namespace for TokenType
    Token token1;
    enum TokenType type1 = TokenType::COMMAND;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleOutputCP(CP_UTF8);

    bool exetime = false;

    // Print the license at start
    typeText(licensePath, 25);

    while (true)
    {
        showPrompt();

        std::string input;
        std::getline(std::cin, input);

        if (input == "exit")
        {
            break;
        }

        std::vector<Token> tokens = tokenize(input);
        std::string commandName = tokens[0].value;
        std::wstring wCommandName = stringToWstring(commandName);
        tokens.erase(tokens.begin());

        Command *command = commandRegistry.getCommand(commandName);
        if (command)
        {
            if (exetime)
            {
                auto start = std::chrono::high_resolution_clock::now();

                command->execute(tokens);

                auto end = std::chrono::high_resolution_clock::now();

                // Execution time in microseconds
                auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

                // Convert duration to milliseconds
                auto duration_ms = duration_us / 1000.0;

                // Convert duration to seconds
                auto duration_s = duration_us / 1000000.0;

                // Convert duration to minutes
                auto duration_min = duration_us / 60000000.0;

                std::cout << "Execution time: " << std::endl;
                std::cout << duration_us << " microseconds" << std::endl;
                std::cout << duration_ms << " milliseconds" << std::endl;
                std::cout << duration_s << " seconds" << std::endl;
                std::cout << duration_min << " minutes" << std::endl;
            }
            else
            {
                command->execute(tokens);
            }
        }
        else if (isExecutable(wCommandName))
        {
            executeCommand(wCommandName);
        }
        else if (std::filesystem::exists(commandName))
        {
            if (std::filesystem::is_regular_file(commandName))
            {
                std::wstring extension = std::filesystem::path(commandName).extension();
                if (extension == L".shl")
                {
                    executeCommandFile(commandName, commandRegistry);
                }
                else
                {
                    std::cerr << "Unknown command: " << commandName << std::endl;
                }
            }
            else
            {
                std::cerr << "Unknown command: " << commandName << std::endl;
            }
        }
        else if (commandName == "exetime")
        {
            exetime = not exetime;
        }
        else if (std::getenv(commandName.c_str()) != nullptr)
        {
            try
            {
                char *envValue = std::getenv(commandName.c_str());

                std::string cmdLine = envValue;

                processCommand(cmdLine, commandRegistry, exetime, tokens);
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
        }
        else
        {
            std::cerr << "Unknown command: " << commandName << std::endl;
        }
    }

    return 0;
}
