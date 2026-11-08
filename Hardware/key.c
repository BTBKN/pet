#include "key.h"
#include "timer.h"

extern volatile uint32_t system_tick;

void Key_Init(Key_t* key, GPIO_TypeDef* port, uint16_t pin) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    key->port = port;
    key->pin = pin;
    key->state = KEY_RELEASED;
    key->last_press_time = 0;
    key->debounce_time = 50;  // 50ms消抖
    
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
