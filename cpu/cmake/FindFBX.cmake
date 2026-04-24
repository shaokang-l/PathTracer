# FindFBX.cmake
# Finds the FBX SDK library
#
# This will define the following variables
#
# FBX_FOUND        - True if the system has FBX SDK
# FBX_INCLUDE_DIRS - FBX SDK include directory
# FBX_LIBRARIES    - FBX SDK libraries

include(FindPackageHandleStandardArgs)

# Try to find FBX SDK in standard locations
find_path(FBX_INCLUDE_DIRS
    NAMES fbxsdk.h
    PATHS
    "$ENV{FBX_ROOT}/include"
    "/Applications/Autodesk/FBX SDK/*/include"
    "/usr/local/include"
    "/usr/include"
    PATH_SUFFIXES
    "fbxsdk"
)

if(FBX_INCLUDE_DIRS)
    # Extract version from include path
    string(REGEX MATCH "FBX SDK/([0-9]+\\.[0-9]+\\.[0-9]+)" FBX_VERSION_MATCH "${FBX_INCLUDE_DIRS}")

    if(FBX_VERSION_MATCH)
        set(FBX_VERSION "${CMAKE_MATCH_1}")
    endif()

    # Find FBX libraries
    if(APPLE)
        set(FBX_LIBRARY_DIRS
            "$ENV{FBX_ROOT}/lib/clang/release"
            "/Applications/Autodesk/FBX SDK/${FBX_VERSION}/lib/clang/release"
        )
        find_library(FBX_LIBRARY
            NAMES libfbxsdk.dylib
            PATHS ${FBX_LIBRARY_DIRS}
        )
    elseif(WIN32)
        set(FBX_LIBRARY_DIRS
            "$ENV{FBX_ROOT}/lib/vs2019/x64/release"
            "C:/Program Files/Autodesk/FBX/FBX SDK/${FBX_VERSION}/lib/vs2019/x64/release"
        )
        find_library(FBX_LIBRARY
            NAMES libfbxsdk.lib
            PATHS ${FBX_LIBRARY_DIRS}
        )
    else()
        set(FBX_LIBRARY_DIRS
            "$ENV{FBX_ROOT}/lib/gcc4/x64/release"
            "/usr/local/lib"
            "/usr/lib"
        )
        find_library(FBX_LIBRARY
            NAMES libfbxsdk.so
            PATHS ${FBX_LIBRARY_DIRS}
        )
    endif()

    if(FBX_LIBRARY)
        set(FBX_LIBRARIES ${FBX_LIBRARY})
        set(FBX_FOUND TRUE)
    endif()
endif()

find_package_handle_standard_args(FBX
    REQUIRED_VARS FBX_INCLUDE_DIRS FBX_LIBRARIES
    VERSION_VAR FBX_VERSION
)