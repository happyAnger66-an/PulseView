include(FindPackageHandleStandardArgs)

# We are testing only a couple of files in the include directories
if (CMAKE_SYSTEM_PROCESSOR MATCHES "(arm64)|(aarch64)")
  find_path(MW_INCLUDE_DIR mw/base/now.h 
        HINTS /opt/cross-tools/aarch64-linux-gnu/include/
        /usr/local/include/)
else()
  find_path(MW_INCLUDE_DIR mw/base/now.h
        HINTS /usr/local/include)
endif()

get_filename_component(MW_ROOT_DIR ${MW_INCLUDE_DIR} DIRECTORY)
set(MW_LIB_DIR ${MW_ROOT_DIR}/lib/mw)

find_library(MW_SHM_LIB libmw_shm.so
    HINTS
    ${MW_LIB_DIR})

find_library(MW_BASE_LIB libmw_base.so
    HINTS
    ${MW_LIB_DIR})

find_library(MW_PROTO_UTIL_LIB libmw_proto_util.so
    HINTS
    ${MW_LIB_DIR})

find_library(MW_COMMON_LIB libmw_common.so
    HINTS
    ${MW_LIB_DIR})
 
find_library(MW_COMMON_PROTOS_LIB libmw_common_protos.so
    HINTS
    ${MW_LIB_DIR})

find_library(MW_SYSTEM_STATS_PROTO_LIB libmw_system_stats_protos.so
    HINTS
    ${MW_LIB_DIR})

set(MW_LIBRARIES ${MW_SHM_LIB} 
${MW_BASE_LIB} 
${MW_PROTO_UTIL_LIB} 
${MW_COMMON_LIB}
${MW_COMMON_PROTOS_LIB}
${MW_SYSTEM_STATS_PROTO_LIB}
)

set(MW_INCLUDE_DIRS ${MW_INCLUDE_DIR})