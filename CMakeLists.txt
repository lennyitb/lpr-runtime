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
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-1.84.0
    GIT_SHALLOW TRUE
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

# --- Catch2 ---
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
    GIT_SHALLOW TRUE
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
target_link_libraries(lpr-cli PRIVATE liblpr)

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
