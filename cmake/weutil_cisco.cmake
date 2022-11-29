# weutil_cisco

add_executable(weutil_cisco
    src/weutil/weutil.cc
)
target_link_libraries(weutil_cisco
    bsp-v2
    gflags
    stdc++fs
    weutil
)
target_include_directories(weutil
    PUBLIC
        src/weutil
)
install(
    TARGETS weutil_cisco
    DESTINATION opt/cisco/bin
    PERMISSIONS OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_EXECUTE
)
