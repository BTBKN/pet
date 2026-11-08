#include "DHT11.h"
#include "Delay.h"
      
/*****************乘风单片机工作室******************
  项目		:	STM32DHT11传感器
  版本		: 	V0.0
  MCU		:	STM32F103C8T6
  哔哩哔哩	:	乘风单片机工作室
  CSDN		:	乘风单片机工作室
****************************************************/	
			
 
void DHT11_Rst(void)	   //复位DHT11
{
	//SET OUTPUT
	DHT11_Mode(OUT);
	//拉低DQ
	DHT11_Low;
	//主机拉低18~30ms
	delay_ms(20);
	//拉高DQ
	DHT11_High;
	//主机拉高10~35us
	delay_us(13);     	
}
 
 
/*==========================================================
Name：		DHT11_Check
Function：	等待DHT11的回应
pars:		无
return：	1:未检测到DHT11的存在
			0:检测到DHT11的存在
notes：		无
==========================================================*/
uint8_t DHT11_Check(void) 	   
{   
	uint8_t retry=0;
	DHT11_Mode(IN);//SET INPUT	 
    while (GPIO_ReadInputDataBit(DHT11_GPIO_PORT,DHT11_GPIO_PIN)&&retry<100)//DHT11会拉低40~80us
	{
		retry++;
		delay_us(1);
	};	 
	if(retry>=100)return 1;
	else retry=0;
    while (!GPIO_ReadInputDataBit(DHT11_GPIO_PORT,DHT11_GPIO_PIN)&&retry<100)//DHT11拉低后会再次拉高40~80us
	{
		retry++;
		delay_us(1);
	};
	if(retry>=100)return 1;	    
	return 0;
}
 
/*==========================================================
Name：		DHT11_Read_Bit
Function：	读取DHT11一位
pars:		无
return：	1/0
notes：		无
==========================================================*/
uint8_t DHT11_Read_Bit(void) 			 
{
 	uint8_t retry=0;
	while(GPIO_ReadInputDataBit(DHT11_GPIO_PORT,DHT11_GPIO_PIN)&&retry<100)//等待变为低电平
	{
		retry++;
		delay_us(1);
	}
	retry=0;
	while(!GPIO_ReadInputDataBit(DHT11_GPIO_PORT,DHT11_GPIO_PIN)&&retry<100)//等待变高电平
	{
		retry++;
		delay_us(1);
	}
	delay_us(40);//等待40us
	if(GPIO_ReadInputDataBit(DHT11_GPIO_PORT,DHT11_GPIO_PIN))return 1;
	else return 0;		   
}
 
 
/*==========================================================
Name：		DHT11_Read_Byte
Function：	读取DHT11一个字节
pars:		无
return：	读到的数据
notes：		无
==========================================================*/
uint8_t DHT11_Read_Byte(void)    
{        
	uint8_t i,dat;
	dat=0;
	for (i=0;i<8;i++) 
	{
		dat<<=1; 
		dat|=DHT11_Read_Bit();
	}						    
	return dat;
}
 
/*==========================================================
Name：		DHT11_Read_Data
Function：	读取DHT11数据
pars:		temp:温度
			humi:湿度
return：	0-读取成功
			1-读取失败
notes：		无
==========================================================*/
uint8_t DHT11_Read_Data(uint8_t *temp_int, uint8_t *humi_int, 
                        uint8_t *temp_dec, uint8_t *humi_dec)    
{
    uint8_t buff[5];
    uint8_t i;
    
    DHT11_Rst(); // 发送起始信号
    
    if(DHT11_Check() == 0) // 等待响应
    {
        for(i = 0; i < 5; i++) // 读取40位数据
        {
            buff[i] = DHT11_Read_Byte();
        }
        
        // 正确的校验和验证（低8位）
        uint8_t checksum = (uint8_t)(buff[0] + buff[1] + buff[2] + buff[3]);
        
        if(checksum == buff[4])
        {
            *humi_int = buff[0];  // 湿度整数部分
            *humi_dec = buff[1];  // 湿度小数部分
            *temp_int = buff[2];  // 温度整数部分
            *temp_dec = buff[3];  // 温度小数部分
            return 0;  // 成功
        }
        else
        {
            return 2;  // 校验失败
        }
    }
    
    return 1;  // 传感器无响应
}
 
/*==========================================================
Name：		DHT11_Init
Function：	初始化DHT11的IO口，同时检测DHT11的存在
pars:		temp:温度
			humi:湿度
return：	0-存在 
			1-不存在
notes：		无
==========================================================*/
uint8_t DHT11_Init(void)
{	 
 	GPIO_InitTypeDef  GPIO_InitStructure;	
 	RCC_APB2PeriphClockCmd(DHT11_GPIO_CLK, ENABLE);	 	//使能PA端口时钟
 	GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;		//端口配置
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	//推挽输出
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);	//初始化IO口
 	GPIO_SetBits(DHT11_GPIO_PORT,DHT11_GPIO_PIN);		//输出高电平
			    
	DHT11_Rst();  //发送起始信号
	return DHT11_Check();//等待DHT11的回应
} 
 
void DHT11_Mode(uint8_t mode)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	if(mode)
	{
		GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	}
	else
	{
		GPIO_InitStructure.GPIO_Pin =  DHT11_GPIO_PIN;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	}
	GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}
