#ifndef __MOTOR_TIMER_H
#define __MOTOR_TIMER_H

#include "stm32f10x.h"                  // Device header
#include <stdbool.h>

#define MAX_MOTOR_TIMER_COUNT 16
#define INVALID_ID 0xFFFF

typedef struct {
    uint16_t id;
    uint16_t hour;
    uint16_t minute;
    uint16_t weight;
    uint8_t valid;                      // 标志位：1-有效，0-无效
} motor_timer;

// 函数声明
bool add_motor_timer(uint16_t id, uint16_t hour, uint16_t minute, uint16_t weight);
bool delete_motor_timer(uint16_t id);
bool update_motor_timer(uint16_t id, uint16_t hour, uint16_t minute, uint16_t weight);
bool queryAll_motor_timer(void);
motor_timer* find_motor_timer_by_id(uint16_t id);
uint16_t get_motor_timer_count(void);

#endif
