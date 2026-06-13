# This file was designed for new directory structure exemplified here:
# https://git.xiaojukeji.com/av/experimental_layout to add path to proto files
# and generate proto libraries.
#
#find_package(OpenSSL REQUIRED)
find_package(Protobuf REQUIRED)
find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

include_guard(GLOBAL)

function(add_protobuf_target TARGET_NAME)
    set(options)
    set(oneValueArgs OUTPUT_DIR NAMESPACE)
    set(multiValueArgs PROTO_FILES IMPORT_DIRS)
    
    cmake_parse_arguments(
        ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
    )
    
    if(NOT ARG_PROTO_FILES)
        message(FATAL_ERROR "No PROTO_FILES specified for target ${TARGET_NAME} ")
    endif()
   
    if(NOT ARG_OUTPUT_DIR)
        set(ARG_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
    endif()
        
    if(NOT EXISTS "${ARG_OUTPUT_DIR}")
        file(MAKE_DIRECTORY "${ARG_OUTPUT_DIR}")
    endif()
    
    # 创建库目标
    add_library(${TARGET_NAME} SHARED)
    
    # 为每个 .proto 文件生成代码
    set(GENERATED_SOURCES)
    set(GENERATED_HEADERS)
    
    foreach(PROTO_FILE ${ARG_PROTO_FILES})
        get_filename_component(PROTO_ABS ${PROTO_FILE} ABSOLUTE)
        get_filename_component(PROTO_NAME_WE ${PROTO_FILE} NAME_WE)
        
        # 设置生成文件路径
        set(GENERATED_SRC "${ARG_OUTPUT_DIR}/${PROTO_NAME_WE}.pb.cc")
        set(GENERATED_HDR "${ARG_OUTPUT_DIR}/${PROTO_NAME_WE}.pb.h")
        
        # 创建生成命令
        add_custom_command(
            OUTPUT ${GENERATED_SRC} ${GENERATED_HDR}
            COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            ARGS
                --cpp_out=${ARG_OUTPUT_DIR}
                --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
                ${ARG_IMPORT_DIRS}
                ${PROTO_ABS}
            DEPENDS ${PROTO_ABS}
            COMMENT "Generating ${PROTO_NAME_WE}.pb.{cc,h}"
            VERBATIM
        )
        
        list(APPEND GENERATED_SOURCES ${GENERATED_SRC})
        list(APPEND GENERATED_HEADERS ${GENERATED_HDR})
    endforeach()
    
    # 添加源文件到目标
    target_sources(${TARGET_NAME} PRIVATE ${GENERATED_SOURCES})
    
    # 设置包含目录
    target_include_directories(${TARGET_NAME}
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
        "$<INSTALL_INTERFACE:include>")
    
    # 链接 Protobuf 库
    target_link_libraries(${TARGET_NAME} PUBLIC ${Protobuf_LIBRARIES})
    
    # 设置命名空间
    #if(ARG_NAMESPACE)
    #    set_target_properties(${TARGET_NAME} PROPERTIES
    #        EXPORT_NAME ${ARG_NAMESPACE}::${TARGET_NAME}
    #    )
    #endif()

    # 将生成的文件保存到父作用域
    set(${TARGET_NAME}_GENERATED_SOURCES ${GENERATED_SOURCES} PARENT_SCOPE)
    set(${TARGET_NAME}_GENERATED_HEADERS ${GENERATED_HEADERS} PARENT_SCOPE)

endfunction()