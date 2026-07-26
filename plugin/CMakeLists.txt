cmake_minimum_required(VERSION 3.24)
project(OBodyNGWeight VERSION 1.5.2 LANGUAGES CXX)

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

# ── CommonLibSSE-NG ────────────────────────────────────────────────────────────
FetchContent_Declare(
    CommonLibSSE
    GIT_REPOSITORY https://github.com/CharmedBaryon/CommonLibSSE-NG.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)
set(BUILD_TESTS        OFF CACHE BOOL "" FORCE)
set(SKSE_SUPPORT_XBYAK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(CommonLibSSE)

include("${commonlibsse_SOURCE_DIR}/cmake/CommonLibSSE.cmake")

# ── Plugin ────────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
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
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/../SKSE/Plugins")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "${OUTPUT_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    COMMENT "Installing OBodyNGWeight.dll into SKSE/Plugins"
)

# ── Unit tests (game-free ShapeCore; no RE::/SKSE) ────────────────────────────
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
    src/ShapeCore.cpp
)
target_include_directories(OBW_Tests PRIVATE src)
target_link_libraries(OBW_Tests PRIVATE doctest::doctest)
# Tests don't need LTO; keep them fast to build.
set_target_properties(OBW_Tests PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
add_test(NAME shapecore COMMAND OBW_Tests)
