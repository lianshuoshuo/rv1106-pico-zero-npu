#ifndef UART_COMM_H
#define UART_COMM_H

#include <stdint.h>

// UART 配置
typedef struct {
    const char *device;  // 设备路径，例如 "/dev/ttyS1"
    int baudrate;        // 波特率，默认 115200
} UartConfig;

// 初始化 UART
int uart_init(const UartConfig *config);

// 发送喂食指令给 STM32
int uart_send_feed_command(const char *pet_id);

// 发送查询状态指令
int uart_query_status();

// 关闭 UART
void uart_deinit();

#endif // UART_COMM_H
