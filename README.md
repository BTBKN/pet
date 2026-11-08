# STM32 ESP8266 Wi-Fi Module

基于 STM32F10x 和 ESP8266 的物联网环境监测与称重项目，通过 MQTT 接入 OneNET。

## 功能

- DHT11 温湿度采集
- HX711 称重与校准
- OLED 数据显示
- DS1302 实时时钟
- 步进电机控制
- ESP8266 Wi-Fi 联网
- OneNET MQTT 数据发布与设备控制

## 开发环境

- Keil uVision 5
- STM32 Standard Peripheral Library
- STM32F10x 系列单片机

打开 `project.uvprojx` 即可进入 Keil 工程。

## 配置设备凭据

项目不会提交真实的 Wi-Fi 密码和 OneNET Token。

1. 复制 `OneNET/user_config.example.h`
2. 将副本重命名为 `OneNET/user_config.h`
3. 填写 Wi-Fi 名称、密码以及 OneNET 产品 ID、Token 和设备名称
4. 重新编译并下载到开发板

`OneNET/user_config.h` 已加入 `.gitignore`，不会被 Git 跟踪。

## 目录

- `Hardware/`：外设驱动
- `Library/`：STM32 标准外设库
- `OneNET/`：ESP8266、MQTT 与 OneNET 接口
- `Start/`：启动文件和 CMSIS
- `System/`：系统与延迟函数
- `Tasks/`：任务相关代码
- `User/`：主程序和中断处理

## 构建产物

Keil 生成的 `Objects/`、`Listings/`、`DebugConfig/` 以及用户工程设置不会上传到 GitHub。
