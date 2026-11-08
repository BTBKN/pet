#include "key.h"
#include "timer.h"
#include "Delay.h"

volatile uint32_t system_tick;

/**
  * 函    数：按键获取键码
  * 参    数：无
  * 返 回 值：按下按键的键码值，范围：0~2，返回0代表没有按键按下
  * 注意事项：此函数是阻塞式操作，当按键按住不放时，函数会卡住，直到按键松手
  */
uint8_t Key_GetNum(void)
{
	uint8_t KeyNum = 0;		//定义变量，默认键码值为0
	
	if (GPIO_ReadInputDataBit(Key1_GPIO_Port, Key1_Pin) == 0)			//读PA8输入寄存器的状态，如果为0，则代表按键1按下
	{
		Delay_ms(20);											//延时消抖
		while (GPIO_ReadInputDataBit(Key1_GPIO_Port, Key1_Pin) == 0);	//等待按键松手
		Delay_ms(20);											//延时消抖
		KeyNum = 1;												//置键码为1
	}
	
	return KeyNum;			//返回键码值，如果没有按键按下，所有if都不成立，则键码为默认值0
}

void Key_Init(Key_t* key, GPIO_TypeDef* port, uint16_t pin) {
    GPIO_InitTypeDef GPIO_InitStructure;

    key->port = port;
    key->pin = pin;
    key->state = KEY_RELEASED;
    key->last_press_time = 0;
    key->debounce_time = 50;  // 50ms消抖
    key->press_handled = 0;

    if(port == GPIOA) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    } else if(port == GPIOB) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    } else if(port == GPIOC) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    } else if(port == GPIOD) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    }

    GPIO_InitStructure.GPIO_Pin = pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(port, &GPIO_InitStructure);
}

KeyState_t Key_GetState(Key_t* key) {
    uint8_t current_state = GPIO_ReadInputDataBit(key->port, key->pin);
    
    switch(key->state) {
        case KEY_RELEASED:
            if(current_state == 0) {  // 按键按下（低电平）
                key->last_press_time = system_tick;
                key->state = KEY_PRESSED;
            }
            break;
            
        case KEY_PRESSED:
            if(current_state == 0) {
                if(system_tick - key->last_press_time >= key->debounce_time) {
                    key->state = KEY_HOLD;
                }
            } else {
                key->state = KEY_RELEASED;
            }
            break;
            
        case KEY_HOLD:
            if(current_state == 1) {  // 按键释放（高电平）
                key->state = KEY_RELEASED;
                key->press_handled = 0;  // 释放后重置标志，允许下次按下触发
            }
            break;
    }
    
    return key->state;
}

uint8_t Key_IsPressed(Key_t* key) {
    KeyState_t state = Key_GetState(key);
    return (state == KEY_PRESSED || state == KEY_HOLD);
}

uint8_t Key_IsReleased(Key_t* key) {
    return (Key_GetState(key) == KEY_RELEASED);
}

uint8_t Key_OnPressed(Key_t* key) {
    Key_GetState(key);
    if(key->state == KEY_PRESSED && !key->press_handled) {
        key->press_handled = 1;
        return 1;
    }
    return 0;
}
