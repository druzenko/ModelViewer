cmake_minimum_required(VERSION 3.23)

include(FetchContent)
include(cmake/shaders.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Only \"Debug\" and \"Release\" build configurations configured" FORCE)
project(ModelViewer)

FetchContent_Declare(
    DirectXTex
    GIT_REPOSITORY https://github.com/microsoft/DirectXTex
    GIT_TAG        oct2025
)

set(BUILD_TOOLS OFF CACHE BOOL "Do not build tex command-line tools." FORCE)
set(BUILD_DX11 OFF CACHE BOOL "Do not build with DirectX11 Runtime support." FORCE)
FetchContent_Declare(
    DirectXHeaders
    GIT_REPOSITORY https://github.com/microsoft/DirectX-Headers
    GIT_TAG        v1.619.1
)

set(ASSIMP_INSTALL OFF CACHE BOOL "Use assimp as a submodule." FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "Do not build exporters." FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "Do not build all importers." FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "Do not build all importers." FORCE) 
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp/
    GIT_TAG        v6.0.4
)

FetchContent_Declare(
    WinPixEventRuntime
    URL                    https://www.nuget.org/api/v2/package/WinPixEventRuntime/1.0.210818001
    URL_HASH               SHA256=70910c2d58b555693ba340b2f5e92dcb2f3d54690ee98130d2ebf7aef5730307
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(DirectXTex DirectXHeaders assimp WinPixEventRuntime)

foreach(_target assimp UpdateAssimpLibsDebugSymbolsAndDLLs zlibstatic)
    if(TARGET ${_target})
        set_target_properties(${_target} PROPERTIES FOLDER "assimp")
    endif()
endforeach()

foreach(_target DirectX-Guids DirectX-Headers DirectXTex)
    if(TARGET ${_target})
        set_target_properties(${_target} PROPERTIES FOLDER "directx")
    endif()
endforeach()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "ModelViewer/Sources/*.cpp"
    "ModelViewer/Sources/*.h"
)

add_executable(model_viewer ${SOURCES})

target_link_libraries(model_viewer
    PRIVATE
        DirectXTex
        Microsoft::DirectX-Headers
        assimp::assimp
        d3d12.lib
        dxgi.lib
        d3dcompiler.lib
        Shlwapi.lib
        runtimeobject.lib
        dxguid.lib
        "$<$<CONFIG:Debug>:${winpixeventruntime_SOURCE_DIR}/bin/x64/WinPixEventRuntime.lib>"
)

target_include_directories(model_viewer PRIVATE
    "$<$<CONFIG:Debug>:${winpixeventruntime_SOURCE_DIR}/Include/WinPixEventRuntime>"
)

target_compile_definitions(model_viewer PRIVATE
    $<$<CONFIG:Debug>:USE_PIX>
)

add_custom_command(TARGET model_viewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E $<IF:$<CONFIG:Debug>,copy_if_different,true>
        "${winpixeventruntime_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll"
        "$<TARGET_FILE_DIR:model_viewer>/WinPixEventRuntime.dll"
)

target_add_shaders(SHADERS)
target_sources(model_viewer PRIVATE ${SHADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/ModelViewer FILES ${SOURCES} ${SHADERS})

set_property(DIRECTORY ${TEST_CMAKE_CURRENT_SOURCE_DIRDIR} PROPERTY VS_STARTUP_PROJECT model_viewer)

target_compile_definitions(model_viewer PRIVATE UNICODE _UNICODE)
set_target_properties(model_viewer PROPERTIES CXX_STANDARD 23)
set_target_properties(model_viewer PROPERTIES WIN32_EXECUTABLE TRUE)
