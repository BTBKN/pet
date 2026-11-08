#ifndef __STEPMOTOR_H
#define	__STEPMOTOR_H
#include "stm32f10x.h"
#include "delay.h"
#include "stm32f10x.h"                  // Device header


/*****************辰哥单片机设计******************
											STM32
 * 文件			:	28BYJ-48步进电机h文件                   
 * 版本			: V1.0
 * 日期			: 2024.8.31
 * MCU			:	STM32F103C8T6
 * 接口			:	见代码					
 * IP账号		:	辰哥单片机设计（同BILIBILI|抖音|快手|小红书|CSDN|公众号|视频号等）
 * 作者			:	辰哥
 * 工作室		: 异方辰电子工作室
 * 讲解视频	:	https://www.bilibili.com/video/BV1ABHvenE5E/?share_source=copy_web&vd_source=097fdeaf6b6ecfed8a9ff7119c32faf2
 * 官方网站	:	www.yfcdz.cn
 
 * 版本			:	V2.0
 * 优化时间	:	2025.9.23
 * 优化内容	:	优化步进电机驱动方式,将对应相的高低电平互换

**********************BEGIN***********************/		


/***************根据自己需求更改****************/
// 步进电机 GPIO宏定义

#define	MOTOR_CLK				RCC_APB2Periph_GPIOB

#define MOTOR_A 				GPIO_Pin_4
#define MOTOR_B 				GPIO_Pin_5
#define MOTOR_C 				GPIO_Pin_6
#define MOTOR_D 				GPIO_Pin_7
#define MOTOR_PROT 			GPIOB

#define MOTOR_A_LOW	 		GPIO_ResetBits(MOTOR_PROT,MOTOR_A)
#define MOTOR_A_HIGH	 	GPIO_SetBits(MOTOR_PROT,MOTOR_A)

#define MOTOR_B_LOW	 	GPIO_ResetBits(MOTOR_PROT,MOTOR_B)
#define MOTOR_B_HIGH 	GPIO_SetBits(MOTOR_PROT,MOTOR_B)

#define MOTOR_C_LOW 	GPIO_ResetBits(MOTOR_PROT,MOTOR_C)
#define MOTOR_C_HIGH 	GPIO_SetBits(MOTOR_PROT,MOTOR_C)

#define MOTOR_D_LOW  	GPIO_ResetBits(MOTOR_PROT,MOTOR_D)
#define MOTOR_D_HIGH 	GPIO_SetBits(MOTOR_PROT,MOTOR_D)

/* 电机状态机 - 改进版 */
typedef enum {
    MOTOR_IDLE,
    MOTOR_RUNNING_CYCLE,  // 正在执行一个8步周期
    MOTOR_BETWEEN_CYCLES  // 周期间等待
} MotorState_t;

typedef struct {
    MotorState_t state;
    uint8_t direction;        // 0:正转, 1:反转
    uint32_t total_cycles;     // 需要执行的周期数 (64*angle/45)
    uint32_t current_cycle;    // 当前已执行周期数
    uint8_t current_phase;     // 当前周期内的相位(1-8)
    uint8_t step_delay;        // 步进延时(ms)
    uint32_t last_step_time;
    uint8_t cycle_in_progress; // 当前周期是否正在执行
} Motor_t;

/* 使用extern声明全局变量 - 告诉编译器这个变量在其他地方定义 */
extern Motor_t motor;
/*********************END**********************/

void MOTOR_Init(void);
void MOTOR_Rhythm_4_1_8(uint8_t step,uint8_t dly);
void MOTOR_Rhythm_4_1_4(u8 step,u8 dly);
void MOTOR_Direction(u8 dir,u8 num,u8 dly);
void MOTOR_Direction_Angle(u8 dir,u8 num,u16 angle,u8 dly);
void MOTOR_Stop(void);
void Motor_NonBlocking_Init(void);
void Motor_Start(uint8_t direction, uint16_t angle, uint8_t dly);
void Motor_Execute_Cycle(void);
void Motor_Update(uint32_t system_tick);
#endif


