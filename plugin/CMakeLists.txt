cmake_minimum_required(VERSION 3.24)
project(OBodyNGWeight VERSION 1.6.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_SKIP_INSTALL_RULES TRUE)

include(FetchContent)

# ── spdlog ─────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(spdlog)

set(_spdlog_cmake_dir "${CMAKE_BINARY_DIR}/spdlog-cmake")
file(MAKE_DIRECTORY "${_spdlog_cmake_dir}")
file(WRITE "${_spdlog_cmake_dir}/spdlogConfig.cmake"
    "set(spdlog_FOUND TRUE)\n"
    "set(spdlog_VERSION \"1.14.1\")\n"
    "set(spdlog_INCLUDE_DIRS \"${spdlog_SOURCE_DIR}/include\")\n"
)
set(spdlog_DIR "${_spdlog_cmake_dir}" CACHE PATH "" FORCE)

# ── rapidcsv ──────────────────────────────────────────────────────────────────
FetchContent_Declare(
    rapidcsv
    GIT_REPOSITORY https://github.com/d99kris/rapidcsv.git
    GIT_TAG        v8.83
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(rapidcsv)
set(RAPIDCSV_INCLUDE_DIRS "${rapidcsv_SOURCE_DIR}/src" CACHE PATH "" FORCE)

# ── DirectXTK (SimpleMath only) ────────────────────────────────────────────────
# CommonLibSSE-NG v15 does find_package(directxtk CONFIG REQUIRED) and links Microsoft::DirectXTK,
# but only ever includes <SimpleMath.h> (RE/S/State.h). A vcpkg/user-preset install is used when one
# is visible; otherwise (e.g. the fork's GitHub CI, which has no vcpkg step) fetch DirectXTK's sources
# and build JUST SimpleMath.cpp - the full library would compile HLSL shaders and XAudio for nothing.
find_package(directxtk CONFIG QUIET)
if(NOT directxtk_FOUND)
    FetchContent_Declare(
        DirectXTK
        GIT_REPOSITORY https://github.com/microsoft/DirectXTK.git
        GIT_TAG        oct2025
        GIT_SHALLOW    TRUE
        SOURCE_SUBDIR  _no_cmake_  # populate only; don't add_subdirectory the full library
    )
    FetchContent_MakeAvailable(DirectXTK)
    add_library(OBW_DirectXTK_SimpleMath STATIC "${directxtk_SOURCE_DIR}/Src/SimpleMath.cpp")
    target_include_directories(OBW_DirectXTK_SimpleMath
        PUBLIC  "${directxtk_SOURCE_DIR}/Inc"
        PRIVATE "${directxtk_SOURCE_DIR}/Src")
    target_compile_features(OBW_DirectXTK_SimpleMath PUBLIC cxx_std_17)
    add_library(Microsoft::DirectXTK ALIAS OBW_DirectXTK_SimpleMath)

    set(_dxtk_cmake_dir "${CMAKE_BINARY_DIR}/directxtk-cmake")
    file(MAKE_DIRECTORY "${_dxtk_cmake_dir}")
    file(WRITE "${_dxtk_cmake_dir}/directxtkConfig.cmake"
        "set(directxtk_FOUND TRUE)\n"
        "# Microsoft::DirectXTK is defined by the parent project (SimpleMath-only build).\n"
    )
    set(directxtk_DIR "${_dxtk_cmake_dir}" CACHE PATH "" FORCE)
endif()

# ── CommonLibSSE-NG ────────────────────────────────────────────────────────────
FetchContent_Declare(
    CommonLibSSE
    GIT_REPOSITORY https://github.com/alandtse/CommonLibSSE-NG.git
    GIT_TAG        v15.0.0
    GIT_SHALLOW    TRUE
)
set(BUILD_TESTS        OFF CACHE BOOL "" FORCE)
set(SKSE_SUPPORT_XBYAK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(CommonLibSSE)

include("${commonlibsse_SOURCE_DIR}/cmake/CommonLibSSE.cmake")

# ── Plugin ────────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/BodyNet.cpp
    src/WeightManager.cpp
    src/ShapeCore.cpp
    src/PapyrusBindings.cpp
    src/PresetManager.cpp
    src/Config.cpp
)

add_commonlibsse_plugin(${PROJECT_NAME}
    SOURCES ${SOURCES}
    NAME    "OBodyNGWeight"
    AUTHOR  "Fuzi"
)
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_compile_options(${PROJECT_NAME} PRIVATE /W4)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE "${CMAKE_SOURCE_DIR}/src/PCH.h"
)

# ── Copy DLL to Data/SKSE/Plugins ────────────────────────────────────────────
option(OBW_COPY_TO_MOD "Copy a successful DLL build into the live mod folder" ON)
if(OBW_COPY_TO_MOD)
    set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/../SKSE/Plugins")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${OUTPUT_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMENT "Installing OBodyNGWeight.dll into SKSE/Plugins"
    )
endif()

# ── Unit tests (game-free ShapeCore + BodyNet; no RE::/SKSE) ──────────────────
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(doctest)

enable_testing()
add_executable(OBW_Tests
    tests/test_shapecore.cpp
    tests/test_bodynet.cpp
    src/ShapeCore.cpp
    src/BodyNet.cpp
)
target_include_directories(OBW_Tests PRIVATE src)
# The BodyNet tests load the shipped models straight from the repo.
target_compile_definitions(OBW_Tests PRIVATE
    OBW_MODEL_DIR="${CMAKE_SOURCE_DIR}/../SKSE/Plugins/OBodyNGWeight")
target_link_libraries(OBW_Tests PRIVATE doctest::doctest)
# Tests don't need LTO; keep them fast to build.
set_target_properties(OBW_Tests PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
add_test(NAME unit COMMAND OBW_Tests)
