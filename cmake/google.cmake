# google.cmake

find_package(gflags REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-function")
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "^GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=sized-deallocation")
    if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_GREATER_EQUAL "11")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=mismatched-new-delete")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=volatile")
    endif()
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "^Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=mismatched-new-delete")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=implicit-exception-spec-mismatch")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-volatile")
endif()

IF (DEFINED ENV{GLOG_PATH})
    message(STATUS "Using Google glog library from $ENV{GLOG_PATH}")
    include_directories($ENV{GLOG_PATH}/include)
ELSE()
    message(STATUS "Downloading Google glog from GitHub, please make sure that GitHub is reachable")
    FetchContent_Declare(glog
        URL https://github.com/google/glog/archive/refs/tags/v0.4.0.tar.gz)
    FetchContent_MakeAvailable(glog)
ENDIF()    
