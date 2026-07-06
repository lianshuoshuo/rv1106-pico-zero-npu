cmake_minimum_required(VERSION 3.10)
project(pet_feeder VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 交叉编译配置（开发时可注释，板端编译时启用）
if(CMAKE_CROSSCOMPILING)
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR arm)
endif()

# 编译选项
add_compile_options(-Wall -Wextra -O2)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party
)

# 源文件
set(SOURCES
    src/main.cpp
    src/video_stream.cpp
    src/rtsp_server.cpp
    src/rknn_detector.cpp
    src/reid_module.cpp
    src/food_detector.cpp
    src/mqtt_client.cpp
    src/uart_comm.cpp
    src/business_logic.cpp
)

# 可执行文件
add_executable(pet_feeder ${SOURCES})

# 链接库（根据实际 SDK 调整路径）
# RV1106 SDK 库路径通常在 /usr/lib 或 SDK 的 lib 目录
target_link_libraries(pet_feeder
    pthread
    # RKMPI 相关库
    # rockchip_mpi
    # rkaiq
    # easymedia
    # RKNN 运行库
    # rknnrt
    # OpenCV（如果使用 opencv-mobile）
    # opencv_core
    # opencv_imgproc
    # MQTT 库
    # paho-mqtt3c
    # SQLite3
    # sqlite3
)

# 安装规则
install(TARGETS pet_feeder DESTINATION /userdata/bin)
install(DIRECTORY config/ DESTINATION /userdata/config)
install(DIRECTORY models/ DESTINATION /userdata/models)
install(DIRECTORY iq/ DESTINATION /userdata/iq)

# 构建信息
message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
