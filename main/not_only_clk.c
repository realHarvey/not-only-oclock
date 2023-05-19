#include <string.h>
#include <stdio.h>
#include <time.h>
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

#define WIFI_SSID   "Linux01"
#define WIFI_PASSWD "mmmmmmmm"
#define SERVER_IP   "192.168.137.1"
#define SERVER_PORT  12345

#define BUFFER_SIZE  1024
#define delay_ms(ms) esp_rom_delay_us(ms * 1000)

void uart1_init()
{
        uart_config_t uart1_cfg = {
                .baud_rate = 115200,
                .data_bits = 8,
                .parity    = 0,
                .stop_bits = 1,
                .flow_ctrl = 0,
                .source_clk = UART_SCLK_APB
        };

        uart_driver_delete(UART_NUM_1);
        uart_param_config(UART_NUM_1, &uart1_cfg);
        uart_set_pin(UART_NUM_1, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        uart_driver_install(UART_NUM_1, BUFFER_SIZE, 0, 0, 0, 0);
}

void uart1_transfer(char *msg)
{
        uart_write_bytes(UART_NUM_1, msg, strlen(msg));
}

void connect_to_wifi()
{
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        wifi_config_t server = {
                .sta = {
                        .ssid = WIFI_SSID,
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
        int ret, len;
        char rx_buffer[BUFFER_SIZE];
        struct sockaddr_in server_addr;

retry:
        cfd = socket(AF_INET, SOCK_STREAM, 0);
        if (cfd < 0) {
                ESP_LOGE("TCP", "create socket");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                goto retry;
        }
        // connect to server
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
        ret = connect(cfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret < 0) {
                ESP_LOGE("TCP", "connect");
                goto retry;
        }
        ESP_LOGI("TCP","成功连接到服务器.");

        while (1) {
                len = recv(cfd, rx_buffer, sizeof(rx_buffer) - 1, 0);
        }
        
        vTaskDelete(NULL);
}

void app_main(void)
{
        time_t now = 0;
        struct tm time_info = { 0 };

        esp_netif_init();
        esp_event_loop_create_default();
        esp_netif_create_default_wifi_sta();
        nvs_flash_init();
        connect_to_wifi();
        ntp_link_init();
        xTaskCreate(app_tcp_client, "app_tcp_client", 4096, NULL, 5, NULL);
        
        while(1) {
                time(&now);
                localtime_r(&now, &time_info);
                ESP_LOGI("TIME", "%d年 %d月 %d日 %d时 %d分 %d秒",
                        time_info.tm_year%100, time_info.tm_mon+1,
                        time_info.tm_mday, time_info.tm_hour,
                        time_info.tm_min, time_info.tm_sec);
                delay_ms(1000);
        }
}
