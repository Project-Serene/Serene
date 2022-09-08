/*

    Serene Compiler



*/
#ifndef LUAU_SERENECOMPILER_H
#define LUAU_SERENECOMPILER_H

/*

    We need extern "C" here
    to avoid C++ name mangling

 */
#if defined(_WIN32) || defined(__WIN32__)
#    define  MYLIB_EXPORT extern "C" __declspec(dllexport)
#elif defined(linux) || defined(__linux)
# define MYLIB_EXPORT
#else
#define MYLIB_EXPORT
#endif

MYLIB_EXPORT bool CompileFile(const char *source_file,const char *output_file);

#undef MYLIB_EXPORT

#endif //LUAU_SERENECOMPILER_H
