cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(lzvn LANGUAGES C ASM VERSION 1.0.0)
set(lzvn_FULL_VERSION "${lzvn_VERSION_MAJOR}.${lzvn_VERSION_MINOR}.${lzvn_VERSION_PATCH}")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH} )

include(FeatureSummary)
include(GetGitRevisionDescription)
include(CMakeDetermineASMCompiler)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})

set(CMAKE_ASM_CREATE_SHARED_LIBRARY ${CMAKE_C_CREATE_SHARED_LIBRARY})
if(WIN32)
    add_library(FastCompression SHARED lzvn_decode.S lzvn_encode.S shlibmain.c)
else()
    add_library(FastCompression SHARED lzvn_decode.S lzvn_encode.S)
endif()
set_target_properties(FastCompression PROPERTIES
    LINKER_LANGUAGE C
    VERSION ${lzvn_VERSION_MAJOR}.${lzvn_VERSION_MINOR}
    SOVERSION ${lzvn_VERSION_MAJOR})
add_library(FastCompression_static STATIC lzvn_decode.S lzvn_encode.S)
set_target_properties(FastCompression_static PROPERTIES
    OUTPUT_NAME FastCompression)

git_describe(GIT_FULL_VERSION "--tags")
if(GIT_FULL_VERSION)
    # development build, i.e. from a git working copy.
    # Get the current commit version using `git describe` and
    # strip the leading v plus the part that matches the version
    # declared in the project definition above.
    string(REPLACE "v${lzvn_FULL_VERSION}" "" COMMIT_VERSION "${GIT_FULL_VERSION}")
    if(NOT ${COMMIT_VERSION} MATCHES ${GIT_FULL_VERSION})
        # `git describe` agrees with lzvn_VERSION_STRING;
        # make an atomic version string by appending the additional info
        string(APPEND lzvn_FULL_VERSION ${COMMIT_VERSION})
    else()
        # `git describe` provides a seemingly unrelated version string;
        # append it in parentheses.
        string(APPEND lzvn_FULL_VERSION " (${GIT_FULL_VERSION})")
    endif()
    message(STATUS "Setting lzvn version to: ${lzvn_FULL_VERSION}")
    configure_file(lzvn_fullversion.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/lzvn_fullversion.h)
    add_definitions(-DHAS_FULLVERSION)
endif()

add_executable(lzvn lzvn.c)
target_link_libraries(lzvn FastCompression_static)
if(APPLE)
    target_link_libraries(lzvn "-framework IOKit -framework CoreFoundation")
endif()

add_executable(lzvns lzvn.c)
target_link_libraries(lzvns FastCompression)
if(APPLE)
    target_link_libraries(lzvns "-framework IOKit -framework CoreFoundation")
endif()

install(TARGETS FastCompression FastCompression_static lzvn
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(FILES lzvn.h DESTINATION include)
