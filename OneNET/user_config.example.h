#ifndef USER_CONFIG_H
#define USER_CONFIG_H

/* Copy this file to user_config.h and replace the placeholders. */

/* OneNET device credentials */
#define PROID   "YOUR_ONENET_PRODUCT_ID"
#define TOKEN   "YOUR_ONENET_TOKEN"
#define DEVID   "YOUR_DEVICE_NAME"

/* Wi-Fi credentials */
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

#define ESP8266_WIFI_INFO \
    "AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASSWORD "\"\r\n"

#endif
