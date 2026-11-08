#ifndef __DHT11_H
#define __DHT11_H
 
#include "stm32f10x.h"
#include "Delay.h"
 
/*****************乘风单片机工作室******************
  项目		:	STM32光敏电阻传感器
  版本		: 	V0.0
  MCU		:	STM32F103C8T6
  哔哩哔哩	:	乘风单片机工作室
  CSDN		:	乘风单片机工作室
****************************************************/	
 
 
/***************自定义修改****************/
//DHT11引脚宏定义
#define DHT11_GPIO_PORT  GPIOA
#define DHT11_GPIO_PIN   GPIO_Pin_7
#define DHT11_GPIO_CLK   RCC_APB2Periph_GPIOA
/*********************END**********************/
 
//输出状态定义
#define OUT 1
#define IN  0
 
//控制DHT11引脚输出高低电平
#define DHT11_Low  GPIO_ResetBits(DHT11_GPIO_PORT,DHT11_GPIO_PIN)
#define DHT11_High GPIO_SetBits(DHT11_GPIO_PORT,DHT11_GPIO_PIN)
 
 
uint8_t DHT11_Init(void);//初始化DHT11
uint8_t DHT11_Read_Data(uint8_t *temp_int, uint8_t *humi_int, 
                        uint8_t *temp_dec, uint8_t *humi_dec)  ;//读取温湿度数据
uint8_t DHT11_Read_Byte(void);//读取一个字节的数据
uint8_t DHT11_Read_Bit(void);//读取一位的数据
void DHT11_Mode(uint8_t mode);//DHT11引脚输出模式控制
uint8_t DHT11_Check(void);//检测DHT11
void DHT11_Rst(void);//复位DHT11   
 
#endif
