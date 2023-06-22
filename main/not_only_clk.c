/*
 * client
 * Anyone-Copyright 2023 何宏波
 */
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi_types.h"
#include "esp_sntp.h"
#include "lwip/sockets.h"

// wifi名和密码
#define WIFI_SSID   "Linux01"
#define WIFI_PASSWD "mmmmmmmm"
// 主机ip和端口
#define SERVER_IP   "192.168.137.1"
#define SERVER_PORT  11451

#define BUFFER_SIZE  6
#define delay_ms(ms) vTaskDelay(ms / 10)

char time_head = 0xFF;
char computer_head = 0xFE;

void uart1_init()
{
        uart_config_t uart1_cfg = {
                .baud_rate = 115200,
                .data_bits = UART_DATA_8_BITS,
                .parity    = UART_PARITY_DISABLE,
                .stop_bits = UART_STOP_BITS_1,
                .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                .source_clk= UART_SCLK_APB
        };
        uart_driver_delete(UART_NUM_1);
        uart_param_config(UART_NUM_1, &uart1_cfg);
        uart_set_pin(UART_NUM_1, 0, 1, -1, -1);
        uart_driver_install(UART_NUM_1, 256, 0, 0, 0, 0);
}

void connect_to_wifi()
{
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        wifi_config_t server = {
                .sta = {
                    .ssid     = WIFI_SSID,
                    .password = WIFI_PASSWD,
                },
        };
        esp_wifi_init(&cfg);
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config(WIFI_IF_STA, &server);
        esp_wifi_start();
        ESP_LOGI("WIFI", "初始化成功.");
        esp_wifi_connect();
        ESP_LOGI("WIFI", "已连接.");
}

void ntp_link_init()
{
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, "cn.pool.ntp.org");
        sntp_init();
        ESP_LOGI("TIME", "正在获取NTP网络时间...");
        delay_ms(5000);
// 中国时区
        setenv("TZ", "GMT-8", 1);
        tzset();
}

void app_tcp_client()
{
        int cfd;
        int ret, len, i;
        char buf_byte;
        char rx_buffer[BUFFER_SIZE];
        struct sockaddr_in server_addr;

retry:
        cfd = socket(AF_INET, SOCK_STREAM, 0);
        if (cfd < 0) {
                ESP_LOGE("TCP", "in create socket");
                delay_ms(500);
                goto retry;
        }
// connect to server
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
        ret = connect(cfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret < 0) {
                ESP_LOGE("TCP", "in connect");
                goto retry;
        }
        ESP_LOGI("TCP","成功连接到服务器.");
// transfer -> 主机信息
        while (1) {
        // 手动recv完成
                for (i = 0; i < BUFFER_SIZE; i++) {
                        len = recv(cfd, &buf_byte, 1, 0);
                        rx_buffer[i] = buf_byte;
                }
        // 帧头0xFE + 主机信息
                uart_write_bytes(UART_NUM_1, (const char *)&computer_head, 1);
                uart_write_bytes(UART_NUM_1, (const char *)rx_buffer, BUFFER_SIZE);
                
                delay_ms(1000);
        }
        
        vTaskDelete(NULL);
}

void app_main(void)
{
        time_t now;
        struct tm time_info = { 0 };

        esp_netif_init();
        esp_event_loop_create_default();
        esp_netif_create_default_wifi_sta();
        nvs_flash_init();
        connect_to_wifi();
        ntp_link_init();
        uart1_init();
        xTaskCreate(app_tcp_client, "app_tcp_client", 4096, NULL, 5, NULL);

// transfer -> 时间信息
        while(1) {
                time(&now);
                localtime_r(&now, &time_info);
                char msg[] = {
                        time_info.tm_year % 100, time_info.tm_mon + 1,
                        time_info.tm_mday, time_info.tm_hour,
                        time_info.tm_min, time_info.tm_sec
                };
        // 帧头0xFF + 时间(from year to sec)
                uart_write_bytes(UART_NUM_1, (const char *)&time_head, 1);
                uart_write_bytes(UART_NUM_1, (const char *)msg, strlen(msg));
                delay_ms(1000);
        }
}

/* debug */
/*
                ESP_LOGI("TIME", "%d年 %d月 %d日 %d时 %d分 %d秒",
                        time_info.tm_year%100, time_info.tm_mon+1,
                        time_info.tm_mday, time_info.tm_hour,
                        time_info.tm_min, time_info.tm_sec);
*/