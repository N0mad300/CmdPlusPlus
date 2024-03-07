# Cmd++

## Table of Contents :

1. Introduction
2. Usage
3. Commands

## 1. Introduction

This project aim to recreate the Windows's cmd console in C++, in a way to easily add new command using a similar organisation for each command

As previously said every command have a similar organisation which is :

- A class for each command with always a public "execute" method and other private methode specific to the command, those are declared in the **command.h** file
- The source code of the methods of each command his write in the **command.cpp** file
- Once the class of the new command is write you need to associate a command name to the class of the command using this line in the **main** function of the **main.cpp** file :

```c++
commandRegistry.registerCommand("commandName", std::make_unique<commandClass>());
```

Don't forget to replace _commandName_ by the real name of the command to write and _commandClass_ by the real class in which the command's methods are defined

## 2. Usage

You can use the console for :

- Use commands
- Execute an executable from his path
- Execute the custom command file of the console (.shl files)
- Use local variable defined

You can define command file (.shl files) by writing one command by line or define a section using "/-`section_name`" for start and "`section_name`-/" for end, you can execute command defined in this section using "**goto** `section_name`", there is an example of command file in the project files

## 3. Commands

Here is the list of the command supported by the console and short description of them :

#### Basics/OS commands :

- `echo <str>` : Write the string of characters that follow the command
- `cd <path_to_directory>` : Displays the name of the current directory or changes the current directory
- `cls` : Clear the console window
- `cmd` / `cmd++` : Open a new window of the console
- `color <hexadecimal_digit>` : Changes the foreground and background colors of the console
- `time` : Display the current time and date of the day
- `timer [[s <seconds>]/[m <minutes>][h <hours>]]` : Start a timer, optionally you can specify the timer values as argument with the following flags, "s" for seconds, "m" for minutes and "h" for hours
- `memstats` : Display RAM usage informations
- `clc` : Activate the calculator mode, use it to made simple calculations (Addition `+`; Substraction `-`; Division `/`; Multiplication `*`)
- `kill <PID>` : Terminate a process using his PID
- `envvar set [<name> <value>]` : Add new local variable, so this variable will be accessible only for the current running session of the app, you can call it by writing the name of the variable in the console, it will try to execute his value like a command
- `envvar get <name>` : Get the value of a variable by his name
- `envvar unset <name>` : Delete the local variable (Make his value empty)

#### Data/Files commands :

- `assoc <file_extension>` : Display path to the executable of the program which is use by default when trying to open file with the specified extension
- `comp <file1> <file2>` / `fc <file1> <file2>`: Compare the size of two files
- `copy <multiples_files_paths>/<folder_path>/<file_path> <destination_path>` : Copy either multiples files (When multiples input files paths arguments) or all the files of a folder (When the path is a folder) or one file to a destination folder (Always the last argument of the command)
- `hexdump <file_path> [-sf [<save_file_path>]]` : Use to generate an hexadecimal view of a given file
- `findstr <file_path> <save_file_path>` : Use to extract all the strings of characters from a given file
- `qs/quicksearch <search_directory (Ex : 'C:\\')> <file_name>` : Use to make a recursive search for a given directory to list paths to all files with a given name or to all files with a specific extension
- `rem <file_path> <regular_expression>` : Searches for all occurrences of a word or regular expression in a file and return the number of occurrences
- `schema dependency <entry_file_path>` : Make simple recursive graph of the local dependencies of a C++ project take the main file as argument
- `schema folder <folder_path>` : Make a recursive graph of a folder and his subfolders

#### Running process/App commands :

- `lsof <PID>` : Display a list of the DLL files currently use by a given running process using his PID
- `memaddress <PID> <output_file>` : List the memory address use by the different memory type of a running process using his PID, you can optionally specify a path to an output file in which write the memory address
- `rma <PID> <memory_type> <memory_address>` : WIP : Use to get the bytes stored in a given memory address of a given running process, you need to specify the type of memory (code, data, heap, stacks), use with the _memaddress_ command

#### XML (With the "xml" command) :

- `xml cp <folder_path> <xml_parameter_name> <new_value>` : Use to change the value of a given parameter for all the XML files in a folder and his subfolder

#### Encoding :

- `encoding xor <xor_command> <input_file> <output_file> <encryption/decryption_key>` : Use to encode a file using XOR operation with an encryption key
- `gpw <password_length>` : Use to generate a random password for a given length

#### Miscellaneous :

- `random number <length>` : Generate a random number of the given length
- `random coin` : Simulate the toss of a coin and return `heads` or `tails`
