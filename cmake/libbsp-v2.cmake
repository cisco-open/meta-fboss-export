# libbsp-v2

add_library(bsp-v2
    src/libbsp-v2/fpd/fpd.cc
    src/libbsp-v2/fpd/fpd_static.cc
    src/libbsp-v2/idprom/idprom.cc
    src/libbsp-v2/idprom/idprom_factory.cc
    src/libbsp-v2/object/object.cc
    src/libbsp-v2/object/oid.cc
)
target_include_directories( bsp-v2
    PUBLIC
      src/libbsp-v2
      include
      ${json_SOURCE_DIR}/include
      ${date_SOURCE_DIR}/include
)
