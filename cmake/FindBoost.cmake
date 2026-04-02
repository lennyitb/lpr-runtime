# Custom FindBoost.cmake — bridges FetchContent'd Boost to find_package(Boost)
# This is used by SymEngine's find_package(Boost) call when Boost was
# fetched via FetchContent with the modular CMake-based layout.

if(TARGET Boost::headers)
    set(Boost_FOUND TRUE)

    # Collect include directories from all Boost modular libraries
    # Some libraries nest under libs/category/sublibrary/ (e.g. numeric/conversion)
    file(GLOB _boost_lib_dirs "${boost_SOURCE_DIR}/libs/*/include")
    file(GLOB _boost_nested_dirs "${boost_SOURCE_DIR}/libs/*/*/include")
    list(APPEND _boost_lib_dirs ${_boost_nested_dirs})
    set(Boost_INCLUDE_DIRS ${_boost_lib_dirs})
    set(Boost_INCLUDE_DIR "${boost_SOURCE_DIR}/libs/config/include" CACHE PATH "" FORCE)

    # Extract version from boost/version.hpp
    file(STRINGS "${boost_SOURCE_DIR}/libs/config/include/boost/version.hpp"
         _boost_ver_line REGEX "#define BOOST_VERSION ")
    if(_boost_ver_line)
        string(REGEX MATCH "[0-9]+" _boost_ver_int "${_boost_ver_line}")
        math(EXPR Boost_VERSION_MAJOR "${_boost_ver_int} / 100000")
        math(EXPR Boost_VERSION_MINOR "${_boost_ver_int} / 100 % 1000")
        math(EXPR Boost_VERSION_PATCH "${_boost_ver_int} % 100")
        set(Boost_VERSION "${Boost_VERSION_MAJOR}.${Boost_VERSION_MINOR}.${Boost_VERSION_PATCH}")
        set(Boost_VERSION_STRING "${Boost_VERSION}")
    endif()

    # Satisfy any COMPONENTS requests (all header-only for our use)
    foreach(_comp IN LISTS Boost_FIND_COMPONENTS)
        set(Boost_${_comp}_FOUND TRUE)
    endforeach()

    set(Boost_LIBRARIES "")

    if(NOT Boost_FIND_QUIETLY)
        message(STATUS "FindBoost: using FetchContent'd Boost ${Boost_VERSION}")
    endif()
else()
    set(Boost_FOUND FALSE)
    if(Boost_FIND_REQUIRED)
        message(FATAL_ERROR "Boost targets not available from FetchContent")
    endif()
endif()
