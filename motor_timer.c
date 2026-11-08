#include "motor_timer.h"
#include <string.h>

uint16_t cnt_motor_timer = 0;
motor_timer motorTimerList[MAX_MOTOR_TIMER_COUNT];

/**
 * @brief 初始化所有定时器槽位为无效
 */
static void init_motor_timer_list(void) {
    for (uint16_t i = 0; i < MAX_MOTOR_TIMER_COUNT; i++) {
        motorTimerList[i].valid = 0;
        motorTimerList[i].id = INVALID_ID;
    }
    cnt_motor_timer = 0;
}

/**
 * @brief 检查ID是否已存在
 */
static bool is_id_exists(uint16_t id) {
    for (uint16_t i = 0; i < MAX_MOTOR_TIMER_COUNT; i++) {
        if (motorTimerList[i].valid && motorTimerList[i].id == id) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 添加电机定时器
 * @param id 定时器ID
 * @param hour 小时(0-23)
 * @param minute 分钟(0-59)
 * @param weight 权重
 * @return true-成功 false-失败
 */
bool add_motor_timer(uint16_t id, uint16_t hour, uint16_t minute, uint16_t weight) {
    // 参数检查
    if (hour > 23 || minute > 59) {
        return false;  // 时间参数无效
    }
    
    // 检查是否已满
    if (cnt_motor_timer >= MAX_MOTOR_TIMER_COUNT) {
        return false;  // 列表已满
    }
    
    // 检查ID是否已存在
    if (is_id_exists(id)) {
        return false;  // ID已存在
    }
    
    // 查找空闲位置
    for (uint16_t i = 0; i < MAX_MOTOR_TIMER_COUNT; i++) {
        if (!motorTimerList[i].valid) {
            motorTimerList[i].id = id;
            motorTimerList[i].hour = hour;
            motorTimerList[i].minute = minute;
            motorTimerList[i].weight = weight;
            motorTimerList[i].valid = 1;
            cnt_motor_timer++;
            return true;
        }
    }
    
    return false;  // 没有空闲位置
}

/**
 * @brief 删除电机定时器
 * @param id 要删除的定时器ID
 * @return true-成功 false-失败
 */
bool delete_motor_timer(uint16_t id) {
    for (uint16_t i = 0; i < MAX_MOTOR_TIMER_COUNT; i++) {
        if (motorTimerList[i].valid && motorTimerList[i].id == id) {
            motorTimerList[i].valid = 0;
            motorTimerList[i].id = INVALID_ID;
            cnt_motor_timer--;
            return true;
        }
    }
    return false;  // 未找到指定ID
}

/**
 * @brief 更新电机定时器
 * @param id 要更新的定时器ID
 * @param hour 新的小时
 * @param minute 新的分钟
 * @param weight 新的权重
 * @return true-成功 false-失败
 */
bool update_motor_timer(uint16_t id, uint16_t hour, uint16_t minute, uint16_t weight) {
    // 参数检查
    if (hour > 23 || minute > 59) {
        return false;  // 时间参数无效
    }
    
    // 查找并更新
    for (uint16_t i = 0; i < MAX_MOTOR_TIMER_COUNT; i++) {
        if (motorTimerList[i].valid && motorTimerList[i].id == id) {
            motorTimerList[i].hour = hour;
            motorTimerList[i].minute = minute;
            motorTimerList[i].weight = weight;
            return true;
        }
    }
    
    return false;  // 未找到指定ID
}

/**
 * @brief 查询所有电机定时器
 * @return true-成功 false-失败或无数据
 */
bool queryAll_motor_timer(void) {
    if (cnt_motor_timer == 0) {
        return false;  // 没有定时器数据
    }
    
    // 打印或输出所有定时器信息（根据实际需求修改）
    for (uint16_t i = 0; i < MAX_MOTOR_TIMER_COUNT; i++) {
        if (motorTimerList[i].valid) {
            // 这里可以根据实际需要处理输出，比如通过串口打印
            // printf("ID:%d, Time:%02d:%02d, Weight:%d\r\n", 
            //        motorTimerList[i].id, 
            //        motorTimerList[i].hour, 
            //        motorTimerList[i].minute, 
            //        motorTimerList[i].weight);
        }
    }
    
    return true;
}

/**
 * @brief 根据ID查找定时器
 * @param id 要查找的定时器ID
 * @return 指向定时器结构的指针，未找到返回NULL
 */
motor_timer* find_motor_timer_by_id(uint16_t id) {
    for (uint16_t i = 0; i < MAX_MOTOR_TIMER_COUNT; i++) {
        if (motorTimerList[i].valid && motorTimerList[i].id == id) {
            return &motorTimerList[i];
        }
    }
    return NULL;
}

/**
 * @brief 获取当前有效的定时器数量
 * @return 有效定时器数量
 */
uint16_t get_motor_timer_count(void) {
    return cnt_motor_timer;
}

// 初始化列表
__attribute__((constructor)) 
static void auto_init_motor_timer(void) {
    init_motor_timer_list();
}
