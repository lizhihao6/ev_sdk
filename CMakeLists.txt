cmake_minimum_required(VERSION 3.5)
project(ji)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INSTALL_PREFIX "/usr/local/ev_sdk")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# 使用pkg-config查找必要的依赖包
find_package(OpenCV REQUIRED)
find_library(glog REQUIRED)
set(ENV{PKG_CONFIG_PATH} "/usr/local/ev_sdk/3rd/ev_encrypt_module/lib/pkgconfig:/usr/local/ev_sdk/3rd/license/lib/pkgconfig:ENV{PKG_CONFIG_PATH}")
find_package(PkgConfig REQUIRED)
pkg_search_module(EV_ENCRYPT_MODULE REQUIRED ev_encrypt_module)
pkg_search_module(JI_LICENSE REQUIRED ji_license)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rd/cJSON)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rd/wkt_parser)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:/usr/local/ev_sdk/lib")

# YOLOX PART
option(CUDA_USE_STATIC_CUDA_RUNTIME OFF)
find_package(CUDA REQUIRED)
# cuda
include_directories(/usr/local/cuda-10.2/include)
link_directories(/usr/local/cuda-10.2/lib64)
# cudnn
include_directories(/usr/include)
link_directories(/usr/lib/x86_64-linux-gnu)

# tensorrt 
include_directories(/usr/include/x86_64-linux-gnu)
# link_directories(/usr/local/cuda-10.2/TensorRT/v7.2.1.6/lib) (same as cudnn)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Ofast -Wfatal-errors -D_MWAITXINTRIN_H_INCLUDED")

# 编译libji.so
list(APPEND libji_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ji.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/SampleDetector.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/yolox.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/list.c)
add_library (${PROJECT_NAME} SHARED ${libji_SOURCES})
target_include_directories(${PROJECT_NAME}
        PUBLIC ${OpenCV_INCLUDE_DIR}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PUBLIC ${JI_LICENSE_INCLUDE_DIRS}
        PUBLIC ${EV_ENCRYPT_MODULE_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}
        ${OpenCV_LIBS}
        glog
        ${JI_LICENSE_LDFLAGS}
        ${EV_ENCRYPT_MODULE_LDFLAGS}
        cjson
        WKTParser
        # "/usr/local/ev_sdk/3rd/darknet/lib/libdarknet.so"
        nvinfer
        cudart
        )

# 安装SampleDetector所依赖的libdarknet.so, 请在此处将自己依赖的库安装到/usr/local/ev_sdk/lib下
# install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rd/darknet/lib/libdarknet.so DESTINATION lib)
install(FILES DESTINATION lib)



# 以下是EV_SDK所依赖和需要测试接口的程序&库，请勿修改！！！
# --- BEGIN EV_SDK DEPENDENCIES ---
# 编译 JI API 测试程序
add_executable(test-ji-api ${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp)
target_link_libraries(test-ji-api
        ${OpenCV_LIBS}
        cjson
        glog
        ${PROJECT_NAME}
        )
# Install libji_license_impl.so
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rd/license/lib/libji_license_impl.so DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rd/license/lib/license_lib.sig DESTINATION lib)
# Install libev_encrypt_module
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rd/ev_encrypt_module/lib/libev_encrypt_module.so DESTINATION lib)
# Install libji.so
install(TARGETS ${PROJECT_NAME} DESTINATION lib)
# Install test-ji-api
install(TARGETS test-ji-api DESTINATION bin)
# ---  END EV_SDK DEPENDENCIES  ---