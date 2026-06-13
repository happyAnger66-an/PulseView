function(create_library TARGET_NAME)
    add_library(${TARGET_NAME} ${ARGN})
    
    # 设置标准包含模式
    target_include_directories(${TARGET_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/private
    )
   
    message("CMAKE_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}")
    # 设置 C++ 标准
    target_compile_features(${TARGET_NAME} PUBLIC cxx_std_17)
    
    # 设置编译警告
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
    else()
        target_compile_options(${TARGET_NAME} PRIVATE 
            -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()