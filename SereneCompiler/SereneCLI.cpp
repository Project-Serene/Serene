/*

    SereneCLI

    Responsible for:
        - Checking whether we are under a PRO's directory.
        - Checking:
            - Components.TOML
            - Events.TOML
            - Libraries.TOML
            - Lib/{....}.TOML
        - Calling Compiler and:
            - Defining Main Input Lua file

 */

#include <iostream>
#include <stdlib.h>
#include "FileUtils.h"
#include "TOML.h"

inline std::string separator()
{
#ifdef _WIN32
    return "\\";
#else
    return "/";
#endif
}

#include <cstdio>
#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#define SetWorkingDir _chdir
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#define SetWorkingDir chdir
#endif

int main(int argc, char* argv[]) {

    /*
        Getting Current Working Directory
     */

    char  WorkingDirectory[FILENAME_MAX];
    GetCurrentDir(WorkingDirectory,FILENAME_MAX);
    SetWorkingDir(WorkingDirectory);

    /*
        Parsing TOML File for code generation
    */
    toml::table configs;

    try
    {
        configs = toml::parse_file((separator()+"serene.toml").c_str());
        std::cout << configs << "\n";
    }
    catch (const toml::parse_error& err)
    {
        std::cerr << "Parsing failed:\n" << err << "\n";
        return 1;
    }


    return 0;
}