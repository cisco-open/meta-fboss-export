# weutil

add_executable(weutil
    src/weutil/weutil.cc
    src/weutil/WeutilExport.cpp
    fboss/platform/weutil/WeutilSandia.cpp
    fboss/platform/weutil/WeutilLassen.cpp
    fboss/platform/weutil/WeutilConfig.cpp
    fboss/platform/weutil/WeutilPlatform.cpp
    fboss/platform/weutil/SandiaWeutilConfig.cpp
    fboss/platform/weutil/LassenWeutilConfig.cpp
)
target_link_libraries(weutil
    bsp-v2
    gflags
    stdc++fs
)
target_include_directories(weutil
    PUBLIC
        src/weutil
)
install(
    TARGETS weutil
    DESTINATION opt/cisco/bin
    PERMISSIONS OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_EXECUTE
)
