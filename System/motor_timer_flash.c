#include "motor_timer_flash.h"
#include "Store.h"
#include <string.h>
#include <usart.h>

// 存储索引定义
#define STORE_INDEX_MAGIC       0   // 魔数索引
#define STORE_INDEX_COUNT        1   // 数量索引
#define STORE_INDEX_NEXT_ID      2   // 下一个ID索引
#define STORE_INDEX_TIMERS       3   // 定时器数据起始索引
#define TIMER_SIZE_IN_HALFWORDS  5   // 每个定时器占用的半字数 (id,hour,minute,weight,valid 各占1个半字)

// 全局变量
motor_timer g_motorTimers[MAX_MOTOR_TIMER_COUNT];
uint16_t g_motorTimerCount = 0;
uint16_t g_nextId = 1;  // ID从1开始

// 静态函数声明
static void timers_to_storage(void);
static void storage_to_timers(void);
static bool is_id_exists(uint16_t id);
static int find_timer_index_by_id(uint16_t id);
static uint16_t generate_new_id(void);

/**
 * @brief 生成新的唯一ID
 * @return 新的ID，如果无可用ID则返回INVALID_ID
 */
static uint16_t generate_new_id(void)
{
    uint16_t startId = g_nextId;
    uint16_t id = g_nextId;
    
    // 循环查找可用的ID
    do {
        // 检查ID是否已存在
        bool id_exists = false;
        for (uint16_t i = 0; i < MAX_MOTOR_TIMER_COUNT; i++)
        {
            if (g_motorTimers[i].valid && g_motorTimers[i].id == id)
            {
                id_exists = true;
                break;
            }
        }
        
        if (!id_exists)
        {
            // 找到可用的ID
            g_nextId = id + 1;
            if (g_nextId > 0xFFFE)  // 避免溢出，保留0xFFFF作为无效ID
            {
                g_nextId = 1;
            }
            return id;
        }
        
        id++;
        if (id > 0xFFFE)
        {
            id = 1;
        }
    } while (id != startId);  // 循环一圈还没找到可用ID
    
    return INVALID_ID;  // 无可用ID
}

/**
 * @brief 初始化电机定时器模块，从FLASH加载数据
 */
void MotorTimer_Flash_Init(void)
{
    // 首先调用Store_Init初始化存储模块
    Store_Init();
    
    // 从FLASH加载数据
    MotorTimer_Flash_Load();
}

/**
 * @brief 从FLASH加载定时器数据到内存
 */
void MotorTimer_Flash_Load(void)
{
    uint16_t magic;
    uint16_t i;
    
    // 读取魔数
    magic = Store_Data[STORE_INDEX_MAGIC];
    
    // 检查是否是第一次使用
    if (magic != 0xA5A5)
    {
        // 第一次使用，初始化所有数据为0
        for (i = 0; i < MAX_MOTOR_TIMER_COUNT; i++)
        {
            g_motorTimers[i].id = 0;
            g_motorTimers[i].hour = 0;
            g_motorTimers[i].minute = 0;
            g_motorTimers[i].weight = 0;
            g_motorTimers[i].valid = 0;
        }
        g_motorTimerCount = 0;
        g_nextId = 1;
        
        // 保存初始状态到FLASH
        MotorTimer_Flash_Save();
    }
    else
    {
        // 从Store_Data中恢复数据
        g_motorTimerCount = Store_Data[STORE_INDEX_COUNT];
        g_nextId = Store_Data[STORE_INDEX_NEXT_ID];
        
        // 限制最大数量
        if (g_motorTimerCount > MAX_MOTOR_TIMER_COUNT)
        {
            g_motorTimerCount = MAX_MOTOR_TIMER_COUNT;
        }
        
        // 恢复定时器数据
        for (i = 0; i < MAX_MOTOR_TIMER_COUNT; i++)
        {
            uint16_t base = STORE_INDEX_TIMERS + i * TIMER_SIZE_IN_HALFWORDS;
            
            g_motorTimers[i].id = Store_Data[base];
            g_motorTimers[i].hour = Store_Data[base + 1];
            g_motorTimers[i].minute = Store_Data[base + 2];
            g_motorTimers[i].weight = Store_Data[base + 3];
            g_motorTimers[i].valid = (uint8_t)Store_Data[base + 4];
        }
    }
}

/**
 * @brief 保存内存中的定时器数据到FLASH
 */
void MotorTimer_Flash_Save(void)
{
    uint16_t i;
    
    // 写入魔数
    Store_Data[STORE_INDEX_MAGIC] = 0xA5A5;
    
    // 写入有效数量和下一个ID
    Store_Data[STORE_INDEX_COUNT] = g_motorTimerCount;
    Store_Data[STORE_INDEX_NEXT_ID] = g_nextId;
    
    // 写入定时器数据
    for (i = 0; i < MAX_MOTOR_TIMER_COUNT; i++)
    {
        uint16_t base = STORE_INDEX_TIMERS + i * TIMER_SIZE_IN_HALFWORDS;
        
        Store_Data[base] = g_motorTimers[i].id;
        Store_Data[base + 1] = g_motorTimers[i].hour;
        Store_Data[base + 2] = g_motorTimers[i].minute;
        Store_Data[base + 3] = g_motorTimers[i].weight;
        Store_Data[base + 4] = g_motorTimers[i].valid;
    }
    
    // 调用Store_Save将数据写入FLASH
    Store_Save();
}

/**
 * @brief 检查ID是否已存在
 */
static bool is_id_exists(uint16_t id)
{
    for (uint16_t i = 0; i < MAX_MOTOR_TIMER_COUNT; i++)
    {
        if (g_motorTimers[i].valid && g_motorTimers[i].id == id)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief 根据ID查找定时器索引
 */
static int find_timer_index_by_id(uint16_t id)
{
    for (uint16_t i = 0; i < MAX_MOTOR_TIMER_COUNT; i++)
    {
        if (g_motorTimers[i].valid && g_motorTimers[i].id == id)
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief 添加电机定时器（自动生成ID）
 * @param hour 小时(0-23)
 * @param minute 分钟(0-59)
 * @param weight 权重
 * @return true-成功 false-失败
 */
bool MotorTimer_Flash_Add(uint16_t hour, uint16_t minute, uint16_t weight)
{
    // 参数检查
    if (hour > 23 || minute > 59)
    {
        return false;
    }
    
    // 检查是否已满
    if (g_motorTimerCount >= MAX_MOTOR_TIMER_COUNT)
    {
        return false;
    }
    
    // 生成新的ID
    uint16_t new_id = generate_new_id();
    if (new_id == INVALID_ID)
    {
        return false;  // 无可用ID
    }
    
    // 查找空闲位置
    for (uint16_t i = 0; i < MAX_MOTOR_TIMER_COUNT; i++)
    {
        if (!g_motorTimers[i].valid)
        {
            g_motorTimers[i].id = new_id;
            g_motorTimers[i].hour = hour;
            g_motorTimers[i].minute = minute;
            g_motorTimers[i].weight = weight;
            g_motorTimers[i].valid = 1;
            g_motorTimerCount++;
            
            // 保存到FLASH
            MotorTimer_Flash_Save();
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 删除电机定时器
 * @param id 要删除的定时器ID
 * @return true-成功 false-失败
 */
bool MotorTimer_Flash_Delete(uint16_t id)
{
    int index = find_timer_index_by_id(id);
    
    if (index >= 0)
    {
        g_motorTimers[index].valid = 0;
        g_motorTimers[index].id = 0;
        g_motorTimerCount--;
        
        // 保存到FLASH
        MotorTimer_Flash_Save();
        return true;
    }
    
    return false;
}

/**
 * @brief 更新电机定时器
 * @param id 要更新的定时器ID
 * @param hour 新的小时
 * @param minute 新的分钟
 * @param weight 新的权重
 * @return true-成功 false-失败
 */
bool MotorTimer_Flash_Update(uint16_t id, uint16_t hour, uint16_t minute, uint16_t weight)
{
    // 参数检查
    if (hour > 23 || minute > 59)
    {
        return false;
    }
    
    int index = find_timer_index_by_id(id);
    
    if (index >= 0)
    {
        g_motorTimers[index].hour = hour;
        g_motorTimers[index].minute = minute;
        g_motorTimers[index].weight = weight;
        
        // 保存到FLASH
        MotorTimer_Flash_Save();
        return true;
    }
    
    return false;
}

/**
 * @brief 根据ID查找定时器
 * @param id 要查找的定时器ID
 * @return 指向定时器结构的指针，未找到返回NULL
 */
motor_timer* MotorTimer_Flash_Find(uint16_t id)
{
    int index = find_timer_index_by_id(id);
    
    if (index >= 0)
    {
        return &g_motorTimers[index];
    }
    
    return NULL;
}

/**
 * @brief 获取当前有效的定时器数量
 * @return 有效定时器数量
 */
uint16_t MotorTimer_Flash_GetCount(void)
{
    return g_motorTimerCount;
}

/**
 * @brief 获取所有定时器数组的指针
 * @return 指向定时器数组的指针
 */
motor_timer* MotorTimer_Flash_GetAll(void)
{
    return g_motorTimers;
}

/**
 * @brief 打印所有定时器信息
 */
void MotorTimer_Flash_PrintAll(void)
{
    if (g_motorTimerCount == 0)
    {
        // 这里可以根据实际需要处理输出
        UsartPrintf(USART_DEBUG, "No motor timers found.\r\n");
        return;
    }
    
    for (uint16_t i = 0; i < MAX_MOTOR_TIMER_COUNT; i++)
    {
        if (g_motorTimers[i].valid)
        {
            // 这里可以根据实际需要处理输出，比如通过串口打印
            UsartPrintf(USART_DEBUG, "Timer[%d]: ID=%d, Time=%02d:%02d, Weight=%d\r\n", 
                    i, g_motorTimers[i].id, g_motorTimers[i].hour, 
                    g_motorTimers[i].minute, g_motorTimers[i].weight);
        }
    }
}

/**
 * @brief 清除所有定时器数据
 * @return true-成功 false-失败
 */
bool MotorTimer_Flash_ClearAll(void)
{
    // 清空内存中的数据
    for (uint16_t i = 0; i < MAX_MOTOR_TIMER_COUNT; i++)
    {
        g_motorTimers[i].id = 0;
        g_motorTimers[i].hour = 0;
        g_motorTimers[i].minute = 0;
        g_motorTimers[i].weight = 0;
        g_motorTimers[i].valid = 0;
    }
    g_motorTimerCount = 0;
    g_nextId = 1;
    
    // 保存到FLASH
    MotorTimer_Flash_Save();
    
    return true;
}