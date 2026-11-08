/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	onenet.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-05-08
	*
	*	版本： 		V1.1
	*
	*	说明： 		与onenet平台的数据交互接口层
	*
	*	修改记录：	V1.0：协议封装、返回判断都在同一个文件，并且不同协议接口不同。
	*				V1.1：提供统一接口供应用层使用，根据不同协议文件来封装协议相关的内容。
	************************************************************
	************************************************************
	************************************************************
**/

//单片机头文件
#include "stm32f10x.h"

//网络设备
#include "esp8266.h"
#include "user_config.h"

//协议文件
#include "onenet.h"
#include "mqttkit.h"

//硬件驱动
#include "usart.h"
#include "Delay.h"
#include "stepmotor.h"
#include "motor_timer_flash.h"

//C库
#include <string.h>
#include <stdio.h>

#include "cJSON.h"

extern unsigned char esp8266_buf[512];
extern uint8_t Alarm_flag;
//==========================================================
//	函数名称：	OneNet_DevLink
//
//	函数功能：	与onenet创建连接
//
//	入口参数：	无
//
//	返回参数：	1-成功	0-失败
//
//	说明：		与onenet平台建立连接
//==========================================================
_Bool OneNet_DevLink(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//协议包

	unsigned char *dataPtr;
	int i = 0;
	_Bool status = 1;
	
	UsartPrintf(USART_DEBUG, "OneNet_DevLink\r\n"
							"PROID: %s,	TOKEN: %s, DEVID:%s\r\n"
                        , PROID, TOKEN, DEVID);
	
	if(MQTT_PacketConnect(PROID, TOKEN, DEVID, 256, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//上传平台
		dataPtr = ESP8266_GetIPD(250);									//等待平台响应

		if(dataPtr != NULL)
		{	
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:UsartPrintf(USART_DEBUG, "Tips:	连接成功\r\n");status = 0;break;
					
					case 1:UsartPrintf(USART_DEBUG, "WARN:	连接失败：协议错误\r\n");break;
					case 2:UsartPrintf(USART_DEBUG, "WARN:	连接失败：非法的clientid\r\n");break;
					case 3:UsartPrintf(USART_DEBUG, "WARN:	连接失败：服务器失败\r\n");break;
					case 4:UsartPrintf(USART_DEBUG, "WARN:	连接失败：用户名或密码错误\r\n");break;
					case 5:UsartPrintf(USART_DEBUG, "WARN:	连接失败：非法链接(比如token非法)\r\n");break;
					
					default:UsartPrintf(USART_DEBUG, "ERR:	连接失败：未知错误\r\n");break;
				}
			}
		}
		
		MQTT_DeleteBuffer(&mqttPacket);								//删包
	}
	else
		UsartPrintf(USART_DEBUG, "WARN:	MQTT_PacketConnect Failed\r\n");
	
	return status;
	
}

//==========================================================
//	函数名称：	OneNet_Subscribe
//
//	函数功能：	订阅
//
//	入口参数：	topics：订阅的topic
//				topic_cnt：topic个数
//
//	返回参数：	SEND_TYPE_OK-成功	SEND_TYPE_SUBSCRIBE-需要重发
//
//	说明：		
//==========================================================
void OneNet_Subscribe(const char *topics[], unsigned char topic_cnt)
{
	
	unsigned char i = 0;
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};							//协议包
	
	for(; i < topic_cnt; i++)
		UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topics[i]);
	
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, topics, topic_cnt, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqttPacket);											//删包
	}

}

//==========================================================
//	函数名称：	OneNet_Publish
//
//	函数功能：	发布消息
//
//	入口参数：	topic：发布的主题
//				msg：消息内容
//
//	返回参数：	SEND_TYPE_OK-成功	SEND_TYPE_PUBLISH-需要重送
//
//	说明：		
//==========================================================
void OneNet_Publish(const char *topic, const char *msg)
{

	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};							//协议包
	
	UsartPrintf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);
	
	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg),MQTT_QOS_LEVEL0, 0, 0, &mqttPacket) == 0)
	{
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);					//向平台发送订阅请求
		
		MQTT_DeleteBuffer(&mqttPacket);											//删包
	}

}

//==========================================================
//	函数名称：	OneNet_RevPro
//
//	函数功能：	平台返回数据检测
//
//	入口参数：	dataPtr：平台返回的数据
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNet_RevPro(unsigned char *cmd)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};								//协议包
	
	char *req_payload = NULL;
	char *cmdid_topic = NULL;
	
	unsigned short topic_len = 0;
	unsigned short req_len = 0;
	
	unsigned char type = 0;
	unsigned char qos = 0;
	static unsigned short pkt_id = 0;
	
	short result = 0;
	cJSON *json, *params_json, *feed_json;
	char *dataPtr = NULL;
	char numBuf[10];
	int num = 0;
	
	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_CMD:															//命令下发
			
			result = MQTT_UnPacketCmd(cmd, &cmdid_topic, &req_payload, &req_len);	//解出topic和消息体
			if(result == 0)
			{
				UsartPrintf(USART_DEBUG, "cmdid: %s, req: %s, req_len: %d\r\n", cmdid_topic, req_payload, req_len);
				
				if(MQTT_PacketCmdResp(cmdid_topic, req_payload, &mqttPacket) == 0)	//命令回复组包
				{
					UsartPrintf(USART_DEBUG, "Tips:	Send CmdResp\r\n");
					
					ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//回复命令
					MQTT_DeleteBuffer(&mqttPacket);									//删包
				}
			}
		
		break;
			
		case MQTT_PKT_PUBLISH:  // 接收的Publish消息（包括服务调用和属性设置）
    
			result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
			if(result == 0)
			{
				UsartPrintf(USART_DEBUG, "topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",
													cmdid_topic, topic_len, req_payload, req_len);
				
				// 判断是否是服务调用（topic中包含 /service/）
				if(strstr((char*)cmdid_topic, "/thing/service/") != NULL)
				{
					// ===== 处理服务调用（增删改查定时器）=====
					UsartPrintf(USART_DEBUG, "Service Invoke\r\n");
					
					// 检查payload是否有效
					if(req_payload == NULL)
					{
						UsartPrintf(USART_DEBUG, "ERROR: req_payload is NULL!\r\n");
					}
					else
					{
						UsartPrintf(USART_DEBUG, "req_payload address: %p\r\n", req_payload);
					}
					
					// 解析JSON
					cJSON *root_json = cJSON_Parse(req_payload);
					if(root_json != NULL)
					{
						UsartPrintf(USART_DEBUG, "cJSON parse success\r\n");
						// 获取消息ID
						cJSON *id_json = cJSON_GetObjectItem(root_json, "id");
						char id_str[16] = "0";
						if(id_json != NULL) 
						{
							UsartPrintf(USART_DEBUG, "id_json type: %d\r\n", id_json->type);
							if(id_json->valuestring != NULL)
							{
								strcpy(id_str, id_json->valuestring);
								UsartPrintf(USART_DEBUG, "id_str: %s\r\n", id_str);
							}
							else
							{
								UsartPrintf(USART_DEBUG, "id_json->valuestring is NULL!\r\n");
							}
						}
						else
						{
							UsartPrintf(USART_DEBUG, "id_json is NULL!\r\n");
						}
						
						// 获取params
						cJSON *params_json = cJSON_GetObjectItem(root_json, "params");
						UsartPrintf(USART_DEBUG, "params_json: %p\r\n", params_json);
						
						// 根据服务类型处理
						UsartPrintf(USART_DEBUG, "Checking service type...\r\n");
						if(strstr((char*)cmdid_topic, "AddTimedTask") != NULL)
						{
							// ===== 添加定时任务服务 =====
							UsartPrintf(USART_DEBUG, "Service: AddTimedTask\r\n");
							
							// 解析JSON参数
							cJSON *params = cJSON_GetObjectItem(root_json, "params");
							if(params != NULL)
							{
								cJSON *hour = cJSON_GetObjectItem(params, "hour");
								cJSON *minute = cJSON_GetObjectItem(params, "minute");
								cJSON *foodAmount = cJSON_GetObjectItem(params, "foodAmount");
								
								if(hour != NULL && minute != NULL && foodAmount != NULL)
								{
									uint16_t h = (uint16_t)hour->valueint;
									uint16_t m = (uint16_t)minute->valueint;
									uint16_t weight = (uint16_t)foodAmount->valueint;
									
									// 参数有效性检查
									if(h <= 23 && m <= 59)
									{
										if(MotorTimer_Flash_Add(h, m, weight))
										{
											// 添加成功
											char resp_str[128];
											snprintf(resp_str, sizeof(resp_str), 
													 "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\",\"data\":{\"result\":true}}", id_str);
											
											char resp_topic[128];
											strcpy(resp_topic, (char*)cmdid_topic);
											char *p = strstr(resp_topic, "/invoke");
											if(p != NULL)
											{
												strcpy(p, "/invoke_reply");
												OneNet_Publish(resp_topic, resp_str);
											}
											UsartPrintf(USART_DEBUG, "Add task success: hour=%d, minute=%d, weight=%d\r\n", h, m, weight);
										}
										else
										{
											// 添加失败（已满或ID耗尽）
											char resp_str[128];
											snprintf(resp_str, sizeof(resp_str), 
													 "{\"id\":\"%s\",\"code\":500,\"msg\":\"failed\",\"data\":{\"result\":false}}", id_str);
											
											char resp_topic[128];
											strcpy(resp_topic, (char*)cmdid_topic);
											char *p = strstr(resp_topic, "/invoke");
											if(p != NULL)
											{
												strcpy(p, "/invoke_reply");
												OneNet_Publish(resp_topic, resp_str);
											}
											UsartPrintf(USART_DEBUG, "Add task failed: no available slot\r\n");
										}
									}
									else
									{
										// 参数无效
										char resp_str[128];
										snprintf(resp_str, sizeof(resp_str), 
												 "{\"id\":\"%s\",\"code\":400,\"msg\":\"failed\",\"data\":{\"result\":false}}", id_str);
										
										char resp_topic[128];
										strcpy(resp_topic, (char*)cmdid_topic);
										char *p = strstr(resp_topic, "/invoke");
										if(p != NULL)
										{
											strcpy(p, "/invoke_reply");
											OneNet_Publish(resp_topic, resp_str);
										}
										UsartPrintf(USART_DEBUG, "Add task failed: invalid parameters\r\n");
									}
								}
								else
								{
									// 缺少必要参数
									char resp_str[128];
									snprintf(resp_str, sizeof(resp_str), 
											 "{\"id\":\"%s\",\"code\":400,\"msg\":\"missing required parameters: hour, minute, foodAmount\"}", id_str);
									
									char resp_topic[128];
									strcpy(resp_topic, (char*)cmdid_topic);
									char *p = strstr(resp_topic, "/invoke");
									if(p != NULL)
									{
										strcpy(p, "/invoke_reply");
										OneNet_Publish(resp_topic, resp_str);
									}
								}
							}
						}
						else if(strstr((char*)cmdid_topic, "QueryTimedTasks") != NULL)
						{
							// ===== 查询所有定时任务服务 =====
							UsartPrintf(USART_DEBUG, "Service: QueryTimedTasks\r\n");
							
							char resp_str[512];  // 分配足够大的缓冲区
							char *p = resp_str;
							
							// 开始构建响应
							p += sprintf(p, "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\",\"data\":{\"taskList\":[", id_str);
							
							// 添加任务列表
							int first = 1;
							for(int i = 0; i < MAX_MOTOR_TIMER_COUNT; i++)
							{
								if(g_motorTimers[i].valid)
								{
									if(!first)
									{
										p += sprintf(p, ",");
									}
									p += sprintf(p, 
												 "{\"taskId\":%d,\"hour\":%d,\"minute\":%d,\"foodAmount\":%d}",
												 g_motorTimers[i].id,
												 g_motorTimers[i].hour,
												 g_motorTimers[i].minute,
												 g_motorTimers[i].weight);
									first = 0;
								}
							}
							
							// 完成JSON构建
							sprintf(p, "]}}");
							
							// 发送响应
							char resp_topic[128];
							strcpy(resp_topic, (char*)cmdid_topic);
							p = strstr(resp_topic, "/invoke");
							if(p != NULL)
							{
								strcpy(p, "/invoke_reply");
								OneNet_Publish(resp_topic, resp_str);
							}
							
							UsartPrintf(USART_DEBUG, "Send Reply: %s\r\n", resp_str);
						}
						else if(strstr((char*)cmdid_topic, "UpdateTimedTask") != NULL)
						{
							// ===== 更新定时任务服务 =====
							UsartPrintf(USART_DEBUG, "Service: UpdateTimedTask\r\n");
							
							// 解析JSON参数
							cJSON *params = cJSON_GetObjectItem(root_json, "params");
							if(params != NULL)
							{
								cJSON *taskId = cJSON_GetObjectItem(params, "taskId");
								cJSON *hour = cJSON_GetObjectItem(params, "hour");
								cJSON *minute = cJSON_GetObjectItem(params, "minute");
								cJSON *foodAmount = cJSON_GetObjectItem(params, "foodAmount");
								
								if(taskId != NULL && hour != NULL && minute != NULL && foodAmount != NULL)
								{
									uint16_t id = (uint16_t)taskId->valueint;
									uint16_t h = (uint16_t)hour->valueint;
									uint16_t m = (uint16_t)minute->valueint;
									uint16_t weight = (uint16_t)foodAmount->valueint;
									
									// 参数有效性检查
									if(h <= 23 && m <= 59)
									{
										if(MotorTimer_Flash_Update(id, h, m, weight))
										{
											// 更新成功
											char resp_str[128];
											snprintf(resp_str, sizeof(resp_str), 
													 "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\",\"data\":{\"result\":true}}", id_str);
											
											char resp_topic[128];
											strcpy(resp_topic, (char*)cmdid_topic);
											char *p = strstr(resp_topic, "/invoke");
											if(p != NULL)
											{
												strcpy(p, "/invoke_reply");
												OneNet_Publish(resp_topic, resp_str);
											}
											UsartPrintf(USART_DEBUG, "Update task success: id=%d, hour=%d, minute=%d, weight=%d\r\n", id, h, m, weight);
										}
										else
										{
											// 更新失败（任务不存在）
											char resp_str[128];
											snprintf(resp_str, sizeof(resp_str), 
													 "{\"id\":\"%s\",\"code\":404,\"msg\":\"failed\",\"data\":{\"result\":false}}", id_str);
											
											char resp_topic[128];
											strcpy(resp_topic, (char*)cmdid_topic);
											char *p = strstr(resp_topic, "/invoke");
											if(p != NULL)
											{
												strcpy(p, "/invoke_reply");
												OneNet_Publish(resp_topic, resp_str);
											}
											UsartPrintf(USART_DEBUG, "Update task failed: task id=%d not found\r\n", id);
										}
									}
									else
									{
										// 参数无效
										char resp_str[128];
										snprintf(resp_str, sizeof(resp_str), 
												 "{\"id\":\"%s\",\"code\":400,\"msg\":\"failed\",\"data\":{\"result\":false}}", id_str);
										
										char resp_topic[128];
										strcpy(resp_topic, (char*)cmdid_topic);
										char *p = strstr(resp_topic, "/invoke");
										if(p != NULL)
										{
											strcpy(p, "/invoke_reply");
											OneNet_Publish(resp_topic, resp_str);
										}
									}
								}
								else
								{
									// 缺少必要参数
									char resp_str[128];
									snprintf(resp_str, sizeof(resp_str), 
											 "{\"id\":\"%s\",\"code\":400,\"msg\":\"missing required parameters: taskId, hour, minute, foodAmount\"}", id_str);
									
									char resp_topic[128];
									strcpy(resp_topic, (char*)cmdid_topic);
									char *p = strstr(resp_topic, "/invoke");
									if(p != NULL)
									{
										strcpy(p, "/invoke_reply");
										OneNet_Publish(resp_topic, resp_str);
									}
								}
							}
						}
						else if(strstr((char*)cmdid_topic, "DeleteTimedTask") != NULL)
						{
							// ===== 删除定时任务服务 =====
							UsartPrintf(USART_DEBUG, "Service: DeleteTimedTask\r\n");
							
							// 解析JSON参数
							cJSON *params = cJSON_GetObjectItem(root_json, "params");
							if(params != NULL)
							{
								cJSON *taskId = cJSON_GetObjectItem(params, "taskId");
								
								if(taskId != NULL)
								{
									uint16_t id = (uint16_t)taskId->valueint;
									
									if(MotorTimer_Flash_Delete(id))
									{
										// 删除成功
										char resp_str[128];
										snprintf(resp_str, sizeof(resp_str), 
												 "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\",\"data\":{\"result\":true}}", id_str);
										
										char resp_topic[128];
										strcpy(resp_topic, (char*)cmdid_topic);
										char *p = strstr(resp_topic, "/invoke");
										if(p != NULL)
										{
											strcpy(p, "/invoke_reply");
											OneNet_Publish(resp_topic, resp_str);
										}
										UsartPrintf(USART_DEBUG, "Delete task success: id=%d\r\n", id);
									}
									else
									{
										// 删除失败（任务不存在）
										char resp_str[128];
										snprintf(resp_str, sizeof(resp_str), 
												 "{\"id\":\"%s\",\"code\":404,\"msg\":\"failed\",\"data\":{\"result\":false}}", id_str);
										
										char resp_topic[128];
										strcpy(resp_topic, (char*)cmdid_topic);
										char *p = strstr(resp_topic, "/invoke");
										if(p != NULL)
										{
											strcpy(p, "/invoke_reply");
											OneNet_Publish(resp_topic, resp_str);
										}
										UsartPrintf(USART_DEBUG, "Delete task failed: task id=%d not found\r\n", id);
									}
								}
								else
								{
									// 缺少taskId参数
									char resp_str[128];
									snprintf(resp_str, sizeof(resp_str), 
											 "{\"id\":\"%s\",\"code\":400,\"msg\":\"missing required parameter: taskId\"}", id_str);
									
									char resp_topic[128];
									strcpy(resp_topic, (char*)cmdid_topic);
									char *p = strstr(resp_topic, "/invoke");
									if(p != NULL)
									{
										strcpy(p, "/invoke_reply");
										OneNet_Publish(resp_topic, resp_str);
									}
								}
							}
						}
					}
					else
					{
						UsartPrintf(USART_DEBUG, "cJSON parse failed!\r\n");
						char *error_ptr = cJSON_GetErrorPtr();
						if(error_ptr != NULL)
						{
							UsartPrintf(USART_DEBUG, "cJSON error at: %s\r\n", error_ptr);
						}
						else
						{
							UsartPrintf(USART_DEBUG, "cJSON error: unknown\r\n");
						}
						UsartPrintf(USART_DEBUG, "payload hex dump:\r\n");
						for(int i = 0; i < req_len; i++) {
							UsartPrintf(USART_DEBUG, "%02X ", req_payload[i]);
							if((i+1) % 16 == 0) UsartPrintf(USART_DEBUG, "\r\n");
						}
						UsartPrintf(USART_DEBUG, "\r\n");
					}
					
					// 释放 root_json
					cJSON_Delete(root_json);
					root_json = NULL;
				}
				else
				{
					// 原有的属性设置处理（如 feed）
					json = cJSON_Parse(req_payload);
					params_json = cJSON_GetObjectItem(json,"params");
					feed_json = cJSON_GetObjectItem(params_json,"feed");
					if(feed_json != NULL)
					{
						// 投喂处理逻辑
						if(feed_json->type == cJSON_True){
							Motor_Start(1, 0, 3);
							UsartPrintf(USART_DEBUG,"stepmotor_running\r\n");
						}else{
							motor.state = MOTOR_IDLE;
							MOTOR_Stop();
							UsartPrintf(USART_DEBUG,"stepmotor_stop\r\n");
						}
					}
				cJSON_Delete(json);
			}
			
			// 释放MQTT_UnPacketPublish分配的内存
			if(cmdid_topic != NULL)
			{
				MQTT_FreeBuffer(cmdid_topic);
				cmdid_topic = NULL;
			}
			if(req_payload != NULL)
			{
				MQTT_FreeBuffer(req_payload);
				req_payload = NULL;
			}
		}
	break;
		
	case MQTT_PKT_PUBACK:														//发送Publish消息，平台回复的Ack
		
			if(MQTT_UnPacketPublishAck(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Publish Send OK\r\n");
			
		break;
			
		case MQTT_PKT_PUBREC:														//发送Publish消息，平台回复的Rec，设备需回复Rel消息
		
			if(MQTT_UnPacketPublishRec(cmd) == 0)
			{
				UsartPrintf(USART_DEBUG, "Tips:	Rev PublishRec\r\n");
				if(MQTT_PacketPublishRel(MQTT_PUBLISH_ID, &mqttPacket) == 0)
				{
					UsartPrintf(USART_DEBUG, "Tips:	Send PublishRel\r\n");
					ESP8266_SendData(mqttPacket._data, mqttPacket._len);
					MQTT_DeleteBuffer(&mqttPacket);
				}
			}
		
		break;
			
		case MQTT_PKT_PUBREL:														//收到Publish消息，设备回复Rec后，平台回复的Rel，设备需再回复Comp
			
			if(MQTT_UnPacketPublishRel(cmd, pkt_id) == 0)
			{
				UsartPrintf(USART_DEBUG, "Tips:	Rev PublishRel\r\n");
				if(MQTT_PacketPublishComp(MQTT_PUBLISH_ID, &mqttPacket) == 0)
				{
					UsartPrintf(USART_DEBUG, "Tips:	Send PublishComp\r\n");
					ESP8266_SendData(mqttPacket._data, mqttPacket._len);
					MQTT_DeleteBuffer(&mqttPacket);
				}
			}
		
		break;
		
		case MQTT_PKT_PUBCOMP:													//发送Publish消息，平台返回Rec，设备回复Rel，平台再返回的Comp
		
			if(MQTT_UnPacketPublishComp(cmd) == 0)
			{
				UsartPrintf(USART_DEBUG, "Tips:	Rev PublishComp\r\n");
			}
		
		break;
			
		case MQTT_PKT_SUBACK:														//发送Subscribe消息的Ack
		
			if(MQTT_UnPacketSubscribe(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe OK\r\n");
			else
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe Err\r\n");
		
		break;
			
		case MQTT_PKT_UNSUBACK:													//发送UnSubscribe消息的Ack
		
			if(MQTT_UnPacketUnSubscribe(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT UnSubscribe OK\r\n");
			else
				UsartPrintf(USART_DEBUG, "Tips:	MQTT UnSubscribe Err\r\n");
		
		break;
		
		default:
			result = -1;
		break;
	}
	
	ESP8266_Clear();									//清空缓存
	
	if(result == -1)
		return;
	
	// 注意：MQTT_PKT_PUBLISH 的内存已在 case 中释放
	// 这里只处理 MQTT_PKT_CMD 的情况
	if(type == MQTT_PKT_CMD)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}

}
