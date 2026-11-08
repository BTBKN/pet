#include "stepmotor.h"
#include "usart.h"

/*****************辰哥单片机设计******************
											STM32
 * 文件			:	28BYJ-48步进电机c文件                   
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

/* 在这里真正定义变量 - 只在一个.c文件中定义 */
Motor_t motor;  // 全局变量定义

void MOTOR_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(MOTOR_CLK, ENABLE ); //配置复用时钟
	
	GPIO_InitStructure.GPIO_Pin = MOTOR_A|MOTOR_B|MOTOR_C|MOTOR_D;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(MOTOR_PROT,&GPIO_InitStructure);

	GPIO_ResetBits(GPIOA, MOTOR_A|MOTOR_D|MOTOR_B|MOTOR_C);
}

void MOTOR_Rhythm_4_1_4(uint8_t step,uint8_t dly)
{
	switch(step)
	{
		
		
		case 1 :
			MOTOR_A_HIGH;MOTOR_B_LOW;MOTOR_C_LOW;MOTOR_D_LOW;	//1
		break;
		
		case 2 :
			MOTOR_A_LOW;MOTOR_B_HIGH;MOTOR_C_LOW;MOTOR_D_LOW;	//2
		break;
		
		case 3 :
			MOTOR_A_LOW;MOTOR_B_LOW;MOTOR_C_HIGH;MOTOR_D_LOW;	//3
		break;
		
		case 4 :
			MOTOR_A_LOW;MOTOR_B_LOW;MOTOR_C_LOW;MOTOR_D_HIGH;	//4
		break;		
	}
	Delay_ms(dly);
}

void MOTOR_Rhythm_4_2_4(uint8_t step,uint8_t dly)
{
	switch(step)
	{
		case 0 :
			;;
		break;
		
		case 1 :
			MOTOR_A_HIGH;MOTOR_B_LOW;MOTOR_C_LOW;MOTOR_D_HIGH;	//1
		break;
		
		case 2 :
			MOTOR_A_HIGH;MOTOR_B_HIGH;MOTOR_C_LOW;MOTOR_D_LOW;	//2
		break;
		
		case 3 :
			MOTOR_A_LOW;MOTOR_B_HIGH;MOTOR_C_HIGH;MOTOR_D_LOW;	//3
		break;
		
		case 4 :
			MOTOR_A_LOW;MOTOR_B_LOW;MOTOR_C_HIGH;MOTOR_D_HIGH;	//4
		break;		
	}
	
	Delay_ms(dly);
}

void MOTOR_Rhythm_4_1_8(uint8_t step,uint8_t dly)
{
	switch(step)
	{
		case 0 :
			;;
		break;
		
		case 1 :
			MOTOR_A_HIGH;MOTOR_B_LOW;MOTOR_C_LOW;MOTOR_D_LOW;	//1
		break;
		
		case 2 :
			MOTOR_A_HIGH;MOTOR_B_HIGH;MOTOR_C_LOW;MOTOR_D_LOW;	//2
		break;
		
		case 3 :
			MOTOR_A_LOW;MOTOR_B_HIGH;MOTOR_C_LOW;MOTOR_D_LOW;	//3
		break;
		
		case 4 :
			MOTOR_A_LOW;MOTOR_B_HIGH;MOTOR_C_HIGH;MOTOR_D_LOW;	//4
		break;	
		
		case 5 :
			MOTOR_A_LOW;MOTOR_B_LOW;MOTOR_C_HIGH;MOTOR_D_LOW;	//5
		break;
		
		case 6 :
			MOTOR_A_LOW;MOTOR_B_LOW;MOTOR_C_HIGH;MOTOR_D_HIGH;	//6
		break;
		
		case 7 :
			MOTOR_A_LOW;MOTOR_B_LOW;MOTOR_C_LOW;MOTOR_D_HIGH;	//7
		break;
		
		case 8 :
			MOTOR_A_HIGH;MOTOR_B_LOW;MOTOR_C_LOW;MOTOR_D_HIGH;	//8
		break;			
	}
	
	Delay_ms(dly);
}

void MOTOR_Direction(uint8_t dir,uint8_t num,uint8_t dly)	//方向 节拍 频率
{
	if(dir)
	{
		switch(num)
		{
			case 0:for(uint8_t i=1;i<9;i++){MOTOR_Rhythm_4_1_8(i,dly);}break;
			case 1:for(uint8_t i=1;i<5;i++){MOTOR_Rhythm_4_1_4(i,dly);}break;
			case 2:for(uint8_t i=1;i<5;i++){MOTOR_Rhythm_4_2_4(i,dly);}break;
			default:break;
		}
	}
	else
	{
		switch(num)
		{
			case 0:for(uint8_t i=8;i>0;i--){MOTOR_Rhythm_4_1_8(i,dly);}break;
			case 1:for(uint8_t i=4;i>0;i--){MOTOR_Rhythm_4_1_4(i,dly);}break;
			case 2:for(uint8_t i=4;i>0;i--){MOTOR_Rhythm_4_2_4(i,dly);}break;
			default:break;
		}	
	}
}

void MOTOR_Direction_Angle(uint8_t dir,uint8_t num,u16 angle,uint8_t dly) //方向 节拍  角度 频率
{
		for(u16 i=0;i<(64*angle/45);i++)	//1/512拍,1/512*360 ==每拍对应角度
		{
			MOTOR_Direction(dir,num,dly);	
		}
}

void MOTOR_Stop(void)
{
	GPIO_ResetBits(GPIOA, MOTOR_A|MOTOR_B|MOTOR_C|MOTOR_D);
}

/* 非阻塞电机初始化 */
void Motor_NonBlocking_Init(void) {
    MOTOR_Init();
    motor.state = MOTOR_IDLE;
    motor.current_phase = 1;
    MOTOR_Stop();
}

/* 启动电机非阻塞转动 */
void Motor_Start(uint8_t direction, uint16_t angle, uint8_t dly) {
    if(motor.state == MOTOR_IDLE) {
        motor.state = MOTOR_RUNNING_CYCLE;
        motor.direction = direction;
        motor.step_delay = dly;
        
		// angle=0:一直旋转
		if(angle == 0){
			motor.total_cycles = 10000;
			motor.current_cycle = 0;
		}
		else{
			// 计算总周期数：每个周期8步
			motor.total_cycles = (64 * angle / 45);
			motor.current_cycle = 0;
		}
        
        // 设置起始相位
        motor.current_phase = (direction == 0) ? 1 : 8;
        motor.cycle_in_progress = 0;
    }
}

/* 执行一个完整的8步周期 */
void Motor_Execute_Cycle(void) {
    // 执行一个完整的8步周期
    for(uint8_t i = 1; i <= 8; i++) {
        uint8_t phase;
        if(motor.direction == 0) {  // 正转
            phase = (motor.current_phase + i - 1) % 8;
            if(phase == 0) phase = 8;
        } else {  // 反转
            phase = (motor.current_phase - i + 1 + 8) % 8;
            if(phase == 0) phase = 8;
        }
        
        // 直接调用驱动函数
		MOTOR_Rhythm_4_1_8(phase,motor.step_delay);
        
    }
    
    // 更新相位位置
    if(motor.direction == 0) {
        motor.current_phase += 8;
        if(motor.current_phase > 8) motor.current_phase -= 8;
    } else {
        motor.current_phase -= 8;
        if(motor.current_phase < 1) motor.current_phase += 8;
    }
    
    motor.current_cycle++;
}

/* 电机更新函数 - 改进版 */
void Motor_Update(uint32_t system_tick) {
	//UsartPrintf(USART_DEBUG,"motor.state:%d\r\n",motor.state);
    if(motor.state == MOTOR_IDLE) return;
    
    uint32_t current_time = system_tick;
    
    switch(motor.state) {
        case MOTOR_RUNNING_CYCLE:
            // 立即执行一个完整周期
            Motor_Execute_Cycle();
            motor.last_step_time = current_time;
            // 判断是否执行完所有周期,未完成进入MOTOR_BETWEEN_CYCLES,完成进入MOTOR_IDLE
            if(motor.current_cycle >= motor.total_cycles) {
                motor.state = MOTOR_IDLE;
                MOTOR_Stop();
            } else {
                motor.state = MOTOR_BETWEEN_CYCLES;
            }
            break;
            
        case MOTOR_BETWEEN_CYCLES:
            // 等待指定延时后执行下一个周期
            if(current_time - motor.last_step_time >= motor.step_delay * 8) {
                motor.state = MOTOR_RUNNING_CYCLE;
            }
            break;
            
        default:
            break;
    }
}
