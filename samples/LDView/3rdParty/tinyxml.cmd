cl /MT /nologo /W3 /GX /O2 /D WIN32 /D NDEBUG /D _MBCS /D _LIB /D TIXML_USE_STL /FR /FD /c tiny*.cpp
link -lib /nologo /out:tinyxml_STL.lib tiny*.obj