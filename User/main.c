#include "stm32f10x.h"
#include "Delay.h"
#include "DHT11.h"
#include "OLED.h"
#include "hx711.h"
#include "ds1302.h"
#include "stepmotor.h"
#include "esp8266.h"
#include "onenet.h"
#include "usart.h"
#include "timer.h"
#include "motor_timer_flash.h"
#include "key.h"

/* 全局变量 */
uint8_t temp_int, humi_int;		//温湿度整数部分
uint8_t temp_dec, humi_dec;		//温湿度小数部分
int value_711;					//称重实时原始数
float weight;					//称重实时计算数
char buff[30];					//sprintf缓冲区
float Weights = 200.0;			//称重标准计算数
int32_t Weights_100 = 7876000;	//称重标准原始数
char st[50];					//oled缓冲区
char PUBLIS_BUF[256];
const char devPubTopic[] = "$sys/S97G9F2KHO/stm32/thing/property/post";
const char *devSubTopic[] = {"$sys/S97G9F2KHO/stm32/thing/property/set","$sys/S97G9F2KHO/stm32/thing/service/+/invoke"};
unsigned char *dataPtr = NULL;

/* 按键变量 */
Key_t key1;


/* 任务调度相关 */
#define TASK_NUM 7

typedef struct {
    void (*task_func)(void);
    uint32_t period;
    uint32_t last_run;
} Task_t;

static Task_t tasks[TASK_NUM];				//任务列表
static volatile uint32_t system_tick = 0;	//时间戳

/* 使用TIM3 */
void TIM3_IRQHandler(void) {
    if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
        system_tick++;
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}


/* 任务1: DHT11读取 */
void Task_DHT11_Read(void) {
    static uint32_t last_run = 0;
    static uint8_t dht11_ok = 0;
    
    if(system_tick - last_run < 1000) return;
    last_run = system_tick;
    
    if(!dht11_ok) {
        if(DHT11_Init() == 0) {
            dht11_ok = 1;
        }
        return;
    }
    
    DHT11_Read_Data(&temp_int, &humi_int, &temp_dec, &humi_dec);
}

/* 任务2: 重量读取 */
void Task_Weight_Read(void) {
    static uint32_t last_run = 0;
    static int32_t reset_value = 0;
    static uint8_t calibrated = 0;
    
    if(system_tick - last_run < 200) return;
    last_run = system_tick;
    
    if(!calibrated) {
        reset_value = HX711_GetData();
        calibrated = 1;
        return;
    }
    
    value_711 = HX711_GetData();
    weight = (float)(value_711 - reset_value) * Weights / (float)(Weights_100 - reset_value);
    if(weight < 0) weight = 0;
}

void JsonValue()
{
	uint8_t Temp = temp_int;
	uint8_t Hum = humi_int;
	uint16_t Weight = weight;
	memset(PUBLIS_BUF, 0, sizeof(PUBLIS_BUF));
	
	sprintf(PUBLIS_BUF,"{\"id\":\"123\",\"params\":{\"temp\":{\"value\":%d},\"hum\":{\"value\":%d},\"weight\":{\"value\":%d} }}",
					Temp,Hum,Weight);	
}

/* 任务3: OLED显示 */
void Task_Display(void) {
    static uint32_t last_run = 0;
    
    if(system_tick - last_run < 100) return;
    last_run = system_tick;
    
    // 第一行：温度
    OLED_ShowString(1, 1, "temp:");
    OLED_ShowNum(1, 6, temp_int, 2);
    OLED_ShowChar(1, 8, '.');
    OLED_ShowNum(1, 9, temp_dec, 2);
    OLED_ShowChar(1, 11, 'C');
    
    // 第二行：重量
    OLED_ShowString(2, 1, "weight:");
    if(weight > 0) {
        sprintf(buff, "%.0f g  ", weight);
    } else {
        sprintf(buff, "0 g     ");
    }
    OLED_ShowString(2, 8, buff);
    
    // 第三行：日期
    OLED_ShowString(3, 1, "dat:");
    sprintf(st, "%04d", TimeData.year);
    OLED_ShowString(3, 5, st);
    OLED_ShowString(3, 9, "/");
    sprintf(st, "%02d", TimeData.month);
    OLED_ShowString(3, 10, st);
    OLED_ShowString(3, 12, "/");
    sprintf(st, "%02d", TimeData.day);
    OLED_ShowString(3, 13, st);
    
    // 第四行：时间 + 电机状态
    OLED_ShowString(4, 1, "time:");
    sprintf(st, "%02d", TimeData.hour);
    OLED_ShowString(4, 6, st);
    OLED_ShowString(4, 8, ":");
    sprintf(st, "%02d", TimeData.minute);
    OLED_ShowString(4, 9, st);
    OLED_ShowString(4, 11, ":");
    sprintf(st, "%02d", TimeData.second);
    OLED_ShowString(4, 12, st);
    
    // 显示电机状态
    if(motor.state != MOTOR_IDLE) {
        OLED_ShowString(4, 15, "M");
    } else {
        OLED_ShowString(4, 15, " ");
    }
	
	JsonValue();
	OneNet_Publish(devPubTopic, PUBLIS_BUF);
	ESP8266_Clear();
}

/* 任务4: DS1302读取 */
void Task_DS1302_Read(void) {
    static uint32_t last_run = 0;
    
    if(system_tick - last_run < 500) return;
    last_run = system_tick;
    
    DS1302_read_realTime();
}

/* 任务5: 电机控制 */
void Task_Motor_Control(){
	static uint32_t last_run = 0;
    static uint32_t motor_timer = 0;

	if(system_tick - last_run < 2) return;  // 2ms检查一次
    last_run = system_tick;
	// 更新电机状态
    Motor_Update(system_tick);
}

/* 任务6: 定时任务检测和执行 */
void Task_TimedTask_Check(void) {
    static uint32_t last_run = 0;
    static uint8_t last_minute = 255;
    static uint8_t last_hour = 255;

    if(system_tick - last_run < 100) return;  // 100ms检查一次
    last_run = system_tick;

    uint8_t hour = TimeData.hour;
    uint8_t minute = TimeData.minute;

    if(hour == last_hour && minute == last_minute) return;
    last_hour = hour;
    last_minute = minute;

    for(int i = 0; i < MAX_MOTOR_TIMER_COUNT; i++) {
        if(g_motorTimers[i].valid) {
            if(g_motorTimers[i].hour == hour && g_motorTimers[i].minute == minute) {
                uint16_t angle = g_motorTimers[i].weight * 18;
                if(angle > 0) {
                    Motor_Start(1, angle, 3);
                    UsartPrintf(USART_DEBUG, "Timed task exec: id=%d, h=%d, m=%d, w=%d, angle=%d\r\n",
                               g_motorTimers[i].id, hour, minute, g_motorTimers[i].weight, angle);
                }
            }
        }
    }
}

/* 任务7: 按键检测 */
void Task_Key_Check(void) {
    static uint32_t last_run = 0;

    if(system_tick - last_run < 20) return;  // 20ms检查一次
    last_run = system_tick;

    if(Key_GetNum(&key1)) {
        if(motor.state == MOTOR_IDLE) {
            Motor_Start(1, 720, 3);  // 转2圈(720度)
            UsartPrintf(USART_DEBUG, "Key pressed: Motor started\r\n");
        } else {
            motor.state = MOTOR_IDLE;
			MOTOR_Stop();
            UsartPrintf(USART_DEBUG, "Key pressed: Motor stopped\r\n");
        }
    }
}

/* 任务调度器初始化 */
void Scheduler_Init(void) {
    Timer_Init();

    tasks[0] = (Task_t){Task_DHT11_Read, 1000, 0};
    tasks[1] = (Task_t){Task_Weight_Read, 200, 0};
    tasks[2] = (Task_t){Task_Display, 1000, 0};
    tasks[3] = (Task_t){Task_DS1302_Read, 500, 0};
    tasks[4] = (Task_t){Task_Motor_Control, 2, 0};
    tasks[5] = (Task_t){Task_TimedTask_Check, 100, 0};
    tasks[6] = (Task_t){Task_Key_Check, 20, 0};
}

/* 任务调度器运行 */
void Scheduler_Run(void) {
    for(int i = 0; i < 7; i++) {
        if(system_tick - tasks[i].last_run >= tasks[i].period) {
            tasks[i].last_run = system_tick;
            tasks[i].task_func();
        }
    }
}


int main(void) {
    /* 模块初始化 */
    OLED_Init();
    HX711_Init();
    Motor_NonBlocking_Init();
    DS1302_GPIO_Init();
	Usart_Init();
	ESP8266_Init();
	MotorTimer_Flash_Init();
	MotorTimer_Flash_PrintAll();//串口输出所有定时器
	
	/* 按键初始化 */
	Key_Init(&key1, Key1_GPIO_Port, Key1_Pin);
    while(OneNet_DevLink()) //连接Onenet平台,如果失败等待500ms继续尝试。
	{
		Delay_ms(500);			
	}
    Delay_ms(1000);
    /*订阅主题*/
	OneNet_Subscribe(devSubTopic,2);
	
    /* 任务调度初始化 */
    Scheduler_Init();
    
    /* 主循环 */
    while(1) {
        Scheduler_Run();
		dataPtr = ESP8266_GetIPD(2);
		if(dataPtr != NULL)
			OneNet_RevPro(dataPtr);	
        __WFI();
    }
}