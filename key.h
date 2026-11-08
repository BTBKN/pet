#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

/* 按键定义 */
#define Key1_GPIO_CLK    RCC_APB2Periph_GPIOA
#define Key1_Pin         GPIO_Pin_8
#define Key1_GPIO_Port   GPIOA

/* 按键状态枚举 */
typedef enum {
    KEY_RELEASED = 0,
    KEY_PRESSED = 1,
    KEY_HOLD = 2
} KeyState_t;

/* 按键结构体 */
typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
    KeyState_t state;
    uint32_t last_press_time;
    uint32_t debounce_time;
    uint8_t press_handled;
} Key_t;

/* 函数声明 */
void Key_Init(Key_t* key, GPIO_TypeDef* port, uint16_t pin);
KeyState_t Key_GetState(Key_t* key);
uint8_t Key_IsPressed(Key_t* key);
uint8_t Key_IsReleased(Key_t* key);
uint8_t Key_OnPressed(Key_t* key);
uint8_t Key_GetNum(void);

#endif /* __KEY_H */