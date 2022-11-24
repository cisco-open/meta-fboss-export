# fan_service_lassen

add_library(fan_service_lassen
    fboss/platform/fan_service/LassenFSConfig.cpp
)

target_link_libraries(fan_service_lassen
    bsp-v2
    dl
    gflags
    glog
    stdc++fs
)
target_include_directories(fan_service_lassen
    PUBLIC
      src/fan_service
      fboss/platform/fan_service
      fboss/platform
      include
      ${json_SOURCE_DIR}/include
      ${date_SOURCE_DIR}/include
)
install(
    TARGETS fan_service_lassen
    DESTINATION opt/cisco/lib
    PERMISSIONS OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_EXECUTE
)
