cmake_minimum_required(VERSION 3.10)
project(pet_feeder VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 交叉编译配置
if(CMAKE_CROSSCOMPILING)
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR arm)
endif()

# ─── Luckfox SDK 路径 ────────────────────────────────────────
# 用法：cmake -DLUCKFOX_SDK_PATH=/path/to/luckfox-sdk ..
# 未指定时使用本地 stub 头文件（仅供 IDE 解析，不可在板外运行）
if(DEFINED LUCKFOX_SDK_PATH AND EXISTS "${LUCKFOX_SDK_PATH}")
    set(RKMPI_INC ${LUCKFOX_SDK_PATH}/sysdrv/source/mpp/include)
    set(RKAIQ_INC ${LUCKFOX_SDK_PATH}/sysdrv/source/isp3x/include/rkaiq)
    set(SDK_LIB   ${LUCKFOX_SDK_PATH}/sysdrv/source/mpp/lib)
    message(STATUS "Luckfox SDK: ${LUCKFOX_SDK_PATH}")
    set(HAS_SDK TRUE)
else()
    set(RKMPI_INC ${CMAKE_SOURCE_DIR}/third_party/rkmpi_stub)
    set(RKAIQ_INC ${CMAKE_SOURCE_DIR}/third_party/rkmpi_stub)
    set(SDK_LIB   "")
    message(WARNING "LUCKFOX_SDK_PATH not set — stub headers (IDE only)")
    set(HAS_SDK FALSE)
endif()

# 编译选项
add_compile_options(-Wall -Wextra -O2)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party
    ${RKMPI_INC}
    ${RKAIQ_INC}
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

# ─── 主程序 pet_feeder ──────────────────────────────────────
add_executable(pet_feeder ${SOURCES})

if(HAS_SDK)
    target_link_directories(pet_feeder PRIVATE ${SDK_LIB})
    target_link_libraries(pet_feeder
        pthread
        # RKMPI 核心（板端 /usr/lib 已内置时无需指定路径）
        rockit        # MPI + VI + VENC + VPSS
        rkaiq         # ISP / AEC / AWB
        # RKNN 推理
        rknnrt
        # OpenCV Mobile
        opencv_core opencv_imgproc
        # MQTT
        paho-mqtt3c
    )
else()
    # 本地 macOS 只链 pthread，其余 SDK 库不存在
    target_link_libraries(pet_feeder pthread)
endif()

# ─── 独立验证程序 gc2093_test ───────────────────────────────
add_executable(gc2093_test src/gc2093_test.cpp)

if(HAS_SDK)
    target_link_directories(gc2093_test PRIVATE ${SDK_LIB})
    target_link_libraries(gc2093_test
        pthread
        rockit
        rkaiq
    )
else()
    target_link_libraries(gc2093_test pthread)
endif()

target_include_directories(gc2093_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${RKMPI_INC}
    ${RKAIQ_INC}
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
