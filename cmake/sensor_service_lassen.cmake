# sensor_service_lassen

add_library(sensor_service_lassen
    fboss/platform/sensor_service/LassenSensorConfig.cpp
)

target_link_libraries(sensor_service_lassen
    bsp-v2
    dl
    gflags
    glog
    stdc++fs
)
target_include_directories(sensor_service_lassen
    PUBLIC
      src/sensor_service
      fboss/platform/sensor_service
      fboss/platform
      include
      ${json_SOURCE_DIR}/include
      ${date_SOURCE_DIR}/include
)
install(
    TARGETS sensor_service_lassen
    DESTINATION opt/cisco/lib
    PERMISSIONS OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_EXECUTE
)
