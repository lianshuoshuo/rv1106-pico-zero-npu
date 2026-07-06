#include "uart_comm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

static int g_uart_fd = -1;

int uart_init(const UartConfig *config) {
    if (!config || !config->device) {
        return -1;
    }

    printf("打开 UART: %s, 波特率: %d\n", config->device, config->baudrate);

    // 打开串口设备
    g_uart_fd = open(config->device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (g_uart_fd < 0) {
        perror("无法打开 UART 设备");
        return -1;
    }

    // 配置串口参数
    struct termios options;
    tcgetattr(g_uart_fd, &options);

    // 设置波特率
    speed_t baud;
    switch (config->baudrate) {
        case 9600: baud = B9600; break;
        case 19200: baud = B19200; break;
        case 38400: baud = B38400; break;
        case 57600: baud = B57600; break;
        case 115200: baud = B115200; break;
        default: baud = B115200; break;
    }
    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);

    // 8N1
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // 原始模式
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;

    // 应用配置
    tcsetattr(g_uart_fd, TCSANOW, &options);

    printf("UART 初始化成功\n");
    return 0;
}

int uart_send_feed_command(const char *pet_id) {
    if (g_uart_fd < 0 || !pet_id) {
        return -1;
    }

    // 构造 JSON 指令
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "{\"act\":\"feed\",\"pet\":\"%s\"}\n", pet_id);

    ssize_t written = write(g_uart_fd, cmd, strlen(cmd));
    if (written < 0) {
        perror("UART 写入失败");
        return -1;
    }

    printf("UART 发送喂食指令: %s", cmd);
    return 0;
}

int uart_query_status() {
    if (g_uart_fd < 0) {
        return -1;
    }

    const char *cmd = "{\"act\":\"query\"}\n";
    ssize_t written = write(g_uart_fd, cmd, strlen(cmd));
    if (written < 0) {
        perror("UART 写入失败");
        return -1;
    }

    printf("UART 查询状态\n");
    return 0;
}

void uart_deinit() {
    if (g_uart_fd >= 0) {
        close(g_uart_fd);
        g_uart_fd = -1;
        printf("UART 已关闭\n");
    }
}
