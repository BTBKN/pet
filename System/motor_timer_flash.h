#ifndef __MOTOR_TIMER_FLASH_H
#define __MOTOR_TIMER_FLASH_H

#include "stm32f10x.h"
#include <stdbool.h>

#define MAX_MOTOR_TIMER_COUNT 16
#define INVALID_ID 0xFFFF

// 定时器数据结构
typedef struct {
    uint16_t id;                         // 自动生成的ID
    uint16_t hour;                        // 小时(0-23)
    uint16_t minute;                      // 分钟(0-59)
    uint16_t weight;                       // 权重
    uint8_t valid;                         // 标志位：1-有效，0-无效
} motor_timer;

// 在FLASH中的存储结构
typedef struct {
    uint16_t magic;                        // 魔数，用于校验（0xA5A5）
    uint16_t count;                         // 有效定时器数量
    uint16_t next_id;                       // 下一个可用的ID
    motor_timer timers[MAX_MOTOR_TIMER_COUNT];  // 定时器数组
} motor_timer_storage_t;

// 函数声明
void MotorTimer_Flash_Init(void);
void MotorTimer_Flash_Save(void);
void MotorTimer_Flash_Load(void);

// 增删改查函数（无ID参数版本）
bool MotorTimer_Flash_Add(uint16_t hour, uint16_t minute, uint16_t weight);
bool MotorTimer_Flash_Delete(uint16_t id);
bool MotorTimer_Flash_Update(uint16_t id, uint16_t hour, uint16_t minute, uint16_t weight);
motor_timer* MotorTimer_Flash_Find(uint16_t id);
uint16_t MotorTimer_Flash_GetCount(void);
void MotorTimer_Flash_PrintAll(void);
bool MotorTimer_Flash_ClearAll(void);

// 获取所有有效定时器（用于遍历）
motor_timer* MotorTimer_Flash_GetAll(void);

// 全局变量声明
extern motor_timer g_motorTimers[MAX_MOTOR_TIMER_COUNT];
extern uint16_t g_motorTimerCount;
extern uint16_t g_nextId;

#endif