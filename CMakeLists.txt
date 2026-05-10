cmake_minimum_required(VERSION 3.24)
project(AnimeUpscaler CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Platform not supported. Windows x64 only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

set(wxBUILD_SHARED OFF CACHE BOOL "" FORCE)
set(wxBUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(wxBUILD_TESTS OFF CACHE BOOL "" FORCE)
set(wxBUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(wxUSE_PRECOMP ON CACHE BOOL "" FORCE)


add_subdirectory(subprojects/wxWidgets)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release")

add_executable(${PROJECT_NAME} WIN32
    src/main.cpp
    src/app/App.cpp
    src/app/App.h
    src/ui/MainFrame.cpp
    src/ui/MainFrame.h
    src/config/Config.cpp
    src/config/Config.h
    src/video/Video.cpp
    src/video/Video.h
    src/video/VideoLoader.cpp
    src/video/VideoLoader.h
    src/video/VideoProcessor.cpp
    src/video/VideoProcessor.h
    src/utils/PathUtils.cpp
    src/utils/PathUtils.h
    src/utils/Process.cpp
    src/utils/Process.h
    src/utils/GpuDetect.cpp
    src/utils/GpuDetect.h
    app.manifest
    resources/app.rc
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE wx::core wx::base)
target_precompile_headers(${PROJECT_NAME} PRIVATE <wx/wxprec.h>)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /O1         
        /Os         
        /Gy         
        /GL         
        /Zc:inline  
        /Gw         
    )
    
    target_link_options(${PROJECT_NAME} PRIVATE
        /OPT:REF    
        /OPT:ICF    
        /LTCG       
    )
endif()