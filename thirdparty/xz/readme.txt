To compile your own lzma library:
1) clone https://github.com/tukaani-project/xz
2) change the toolset to msvc_x86 & set the configurartion type to Release
3) to link to the same runtime as spt, add a variable called CMAKE_MSVC_RUNTIME_LIBRARY and set it to MultiThreaded
4) clone the configuration to make the debug lib, but set CMAKE_MSVC_RUNTIME_LIBRARY to MultiThreadedDebug (keep the configuration type as Release)
5) copy all the header files from xz/src/liblzma/api into SourcePauseTool/thirdparty/xz/include
