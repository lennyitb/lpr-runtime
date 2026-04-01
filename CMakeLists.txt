cmake_minimum_required(VERSION 3.20)
project(lpr VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# --- Boost (headers only via full release) ---
set(BOOST_INCLUDE_LIBRARIES multiprecision)
set(BOOST_ENABLE_CMAKE ON)
FetchContent_Declare(
    Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(Boost)

# --- SQLite3 amalgamation ---
FetchContent_Declare(
    sqlite3
    URL https://www.sqlite.org/2024/sqlite-amalgamation-3460000.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_GetProperties(sqlite3)
if(NOT sqlite3_POPULATED)
    FetchContent_Populate(sqlite3)
    add_library(sqlite3 STATIC "${sqlite3_SOURCE_DIR}/sqlite3.c")
    target_include_directories(sqlite3 PUBLIC "${sqlite3_SOURCE_DIR}")
endif()

# --- linenoise ---
FetchContent_Declare(
    linenoise
    URL https://github.com/antirez/linenoise/archive/refs/heads/master.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_GetProperties(linenoise)
if(NOT linenoise_POPULATED)
    FetchContent_Populate(linenoise)
    add_library(linenoise STATIC "${linenoise_SOURCE_DIR}/linenoise.c")
    target_include_directories(linenoise PUBLIC "${linenoise_SOURCE_DIR}")
endif()

# --- FTXUI ---
FetchContent_Declare(
    ftxui
    URL https://github.com/ArthurSonzogni/FTXUI/archive/refs/tags/v5.0.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FTXUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ftxui)

# --- Catch2 ---
FetchContent_Declare(
    Catch2
    URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(Catch2)

# --- liblpr static library ---
file(GLOB_RECURSE LPR_SOURCES "src/*.cpp")
add_library(liblpr STATIC ${LPR_SOURCES})
set_target_properties(liblpr PROPERTIES OUTPUT_NAME lpr)
target_include_directories(liblpr
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(liblpr
    PUBLIC
        Boost::multiprecision
        sqlite3
)

# --- lpr-cli executable ---
add_executable(lpr-cli cli/main.cpp)
target_link_libraries(lpr-cli PRIVATE liblpr linenoise ftxui::screen ftxui::dom)

# --- lpr-tests executable ---
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
add_executable(lpr-tests ${TEST_SOURCES})
target_link_libraries(lpr-tests PRIVATE liblpr Catch2::Catch2WithMain)
target_include_directories(lpr-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(CTest)
include(Catch)
catch_discover_tests(lpr-tests)

# --- Install ---
install(TARGETS liblpr
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(DIRECTORY include/lpr DESTINATION include)
