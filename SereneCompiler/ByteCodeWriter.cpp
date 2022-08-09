#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#else
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

#include <string.h>
#include <iostream>

static std::wstring fromUtf8(const std::string &path) {
    size_t result = MultiByteToWideChar(CP_UTF8, 0, path.data(), int(path.size()), nullptr, 0);

    std::wstring buf(result, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.data(), int(path.size()), &buf[0], int(buf.size()));

    return buf;
}

const std::string header = R"(
    #ifndef SERENE_BYTECODE
    #define SERENE_BYTECODE

    #include "main.h"

    const size_t BYTECODE_SIZE = %d;
    const char BYTECODE[] = {
)""\n\t";

const std::string footer = "\n\t};\n\t#endif";

bool writeByteCode(const char *name, const char *str, unsigned long long size) {
#ifdef _WIN32
    FILE *file = _wfopen(fromUtf8(name).c_str(), L"wb");
#else
    FILE* file = fopen(name.c_str(), "wb");
#endif

    if (!file)
        return false;

    fprintf(file,header.c_str(),size);
    for (unsigned long long i = 0; i < size; ++i) {
        fprintf(file,"0x%02x%s",str[i],((i+1) % 16 == 0 ? ",\n" : ","));
    }
    fprintf(file,"%s", footer.c_str());
    fclose(file);
    return true;
}
