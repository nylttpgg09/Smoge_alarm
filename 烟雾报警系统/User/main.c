
/*
经过串口测试，可以正常上传属性 测试代码   （使用于串口助手直插  esp8266）
AT+MQTTPUB=0,"$oc/devices/{device_id}/sys/properties/report","{\"services\":[{\"service_id\":\"Smoge\"\,\"properties\":{\"Light\":10\,\"Temp\":50\,\"Hum\":60\,\"CO\":10\,\"Smo\":5}}]}",1,0

1.传感器工作正常，esp8266和传感器可以同时使用，pb11修改至pa8
2.可以连接华为云平台，鉴权可以通过，订阅发布存在问题，平台无法接收消息(已解决，请严格按照转义字符)    
3.temp,humi,light_value,smoke_value,co_value,这五个值存放了传感器数据
4.wifi_config.c里修改波特率，默认115200

最新，若要使用该代码，请先修改下列的mtqq对应的配置，以及wifi热点，请参考华为云，会提示怎莫设置



dht的通道由pb11修改至pa8
烟雾的ad转换通道修改至pb0，也就是通道8



*/
#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "beep.h"
#include "key.h"
#include "adc.h"
#include "smg.h"
#include "dht11.h"
#include "oled.h"
#include "wifi_config.h"
#include "wifi_function.h"
#include "string.h"
#include <stdio.h>
#include <string.h>

#define LIGHT_X 5*6    
#define LIGHT_Y 30   

// MQTT配置
#define MQTT_BROKER          												  "地址"
#define MQTT_IP              												  "同上，ping过的"
#define MQTT_PORT            									        "1883"
#define MQTT_CLIENT_ID 																"自己看云平台"
#define MQTT_USERNAME 															  "自己看云平台"
#define MQTT_PASSWORD  																"自己看云平台"


typedef unsigned int u32;
char at_cmd[1024] = {0};

#define ADC_SAMPLES 20
//最好两个地方都要修改，避免出现问题
    const char *ssid     = 		"114514";    													//wifi名称
    const char *wifi_pwd = 		"123457890"; 													//wifi密码
		const char *topic    = 		"$oc/devices/{device_id}/sys/properties/report";        
	//const char *json_payload = "{\\\"services\\\":[{\\\"service_id\\\":\\\"Smoge\\\"\\,\\\"properties\\\":{\\\"Light\\\":10\\,\\\"Temp\\\":50\\,\\\"Hum\\\":60\\,\\\"CO\\\":10\\,\\\"Smg\\\":5}}]}";
		char json_payload[256];

//全局属性定义
	u8 temp_id=1;  	    
	u8 humi_id=1;
	u16 light_value_id=1.0;
	u16 smoke_value_id=1.0, co_value_id=1.0;
	u8 key_id=0;

		u16 smoke_se = 50;  
		u16 co_set = 5; 
    u8 sta = 0;   

//发送数据  ，向云平台发送，若要修改请严格按照格式修改    const char *topic = "$oc/devices/{device——id}/sys/properties/report";
/*
		订阅地址divce_id需要包含两边的{}  ，官方的以及大多数教程没有提到，请一定要加上，通过c代码来发送at指令转义得加\\\

 snprintf(json_payload, sizeof(json_payload),
        "{\\\"services\\\":[{\\\"service_id\\\":\\\"Smoge\\\"\\,\\\"properties\\\":{"
        "\\\"Light\\\":%d\\,"     
        "\\\"Temp\\\":%d\\,"    
        "\\\"Hum\\\":%d\\,"     
        "\\\"CO\\\":%d\\,"        
        "\\\"Smg\\\":%d"          
        "}}]}",
        light_value, temp, humi, co_value, smoke_value);
这个只用修改对应的属性如：Light等，请严格和以上对应。
*/

void send_properties_report(u8 temp, u8 humi, u16 light_value, u16 smoke_value, u16 co_value,u8 key_id)
{

   const char *topic = "$oc/devices/{device——id}/sys/properties/report";
   
    snprintf(json_payload, sizeof(json_payload),
        "{\\\"services\\\":[{\\\"service_id\\\":\\\"Smoge\\\"\\,\\\"properties\\\":{"
        "\\\"Light\\\":%d\\,"     
        "\\\"Temp\\\":%d\\,"    
        "\\\"Hum\\\":%d\\,"     
        "\\\"CO\\\":%d\\,"
				"\\\"Fee\\\":%d\\,"     		
        "\\\"Smg\\\":%d"          
        "}}]}",
        light_value, temp, humi, co_value,key_id, smoke_value);

    snprintf(at_cmd, sizeof(at_cmd), 
        "AT+MQTTPUB=0,\"%s\",\"%s\",1,0", 
        topic, json_payload);
		
		//上传判断
if (ESP8266_Cmd(at_cmd, "OK", "ERROR", 5000)) {
    printf("MQTT Publish Success!\n");
} else {
    printf("MQTT Publish Failed!\n");
}
			
}


//前5步没问题，后面订阅应该是格式问题（已修订，目前均可使用）
void setup_wifi_and_mqtt(void)
{  
    // 1. ESP8266  STA设置 不用动，
    snprintf(at_cmd, sizeof(at_cmd), "AT+CWMODE=1");
    if (ESP8266_Cmd(at_cmd, "OK", "ERROR", 3000))
    {
        printf("STA设置成功\n");
    }
    else
    {
        printf("STA设置失败\n");
        return;
    }
    delay_ms(500); 

    // 2. WiFi连接，可以在上面修改
    snprintf(at_cmd, sizeof(at_cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, wifi_pwd);
    if (ESP8266_Cmd(at_cmd, "WIFI CONNECTED", "ERROR", 10000))
    {
        printf("WiFi连接成功\n");
    }
    else
    {
        printf("WiFi连接失败\n");
        return;
    }
    delay_ms(1000);  // 

    // 3. MQTT配置用户
    // 配置用户:AT+MQTTUSERCFG=0,1,"NULL","用户名","密码",0,0,""
		snprintf(at_cmd, sizeof(at_cmd), "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"", MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
    if (ESP8266_Cmd(at_cmd, "OK", "ERROR", 3000))
    {
        printf("MQTT配置用户成功\n");
    }
    else
    {
        printf("MQTT配置用户失败\n");
        return;
    }
    delay_ms(1000);

    // 4. MQTT配置ClientID
    // 配置:AT+MQTTCLIENTID=0,"ClientID"
    snprintf(at_cmd, sizeof(at_cmd), "AT+MQTTCLIENTID=0,\"%s\"", MQTT_CLIENT_ID);
    if (ESP8266_Cmd(at_cmd, "OK", "ERROR", 3000))
    {
        printf("MQTT ClientID成功\n");
    }
    else
    {
        printf("MQTT ClientID失败\n");
        return;
    }
    delay_ms(1000);
		//mqtt连接                                                          ，ip和端口
    snprintf(at_cmd, sizeof(at_cmd), "AT+MQTTCONN=0,\"%s\",%s,1", MQTT_BROKER, MQTT_PORT);
    if (ESP8266_Cmd(at_cmd, "OK", "ERROR", 5000))
    {
        printf("MQTT连接成功\n");
    }
    else
    {
        printf("MQTT连接失败\n");
        return;
    }
    delay_ms(1000);  // 

		
//转发mqtt订阅
    snprintf(at_cmd, sizeof(at_cmd), "AT+MQTTSUB=0,\"%s\",1", topic);
    if (ESP8266_Cmd(at_cmd, "OK", "ERROR", 3000))
    {
        printf("MQTT成功订阅\n");
    }
    else
    {
        printf("MQTT失败订阅\n");
        return;
    }
    delay_ms(1000);
		BEEP_Alarm(125,254);//响一声 初始化完成
		
}
    u8 temp, humi;//定义温湿度
    u16 light_value;//定义光强
    u16 smoke_value, co_value;//定义烟雾和co浓度
       
    static u16 co_history[5] = {0};  
    static u16 smoke_history[5] = {0};
    static u8 index = 0;              
		u16 smoke_raw;
		u16 co_raw;

//数据处理
void data_pros() 
{

  //dht11读取温湿度
    DHT11_Read_Data(&temp, &humi);
    
		    // CO ?????(?? ADC ??? ? CO ????)
     co_raw = Get_ADC_Value(ADC_Channel_7, 10);
    co_history[index] = co_raw;        // ??????
    co_value = (co_history[0] + co_history[1] + co_history[2] + 
               co_history[3] + co_history[4]) / 5;  // ??????
    co_value =  (co_value * 100) / 4095  - 90;  // ?????? 0~100%
    if (co_value > 100) co_value = 100;
		   // ???????(?? ADC ??? ? ??????)

		delay_ms(5);
     smoke_raw = Get_ADC_Value(ADC_Channel_8, 10);
    smoke_history[index] = smoke_raw;  // ??????
    smoke_value = (smoke_history[0] + smoke_history[1] + smoke_history[2] + 
                   smoke_history[3] + smoke_history[4]) / 5;  // ??????
    smoke_value = (smoke_value * 100) / 4095;  // ????? 0~100%
    if (smoke_value > 100) smoke_value = 100;
			delay_ms(5);
  //光强浓度转化
    light_value = Get_ADC_Value(ADC_Channel_9, 10);
    light_value = (light_value * 100) / 4095;    
    if (light_value > 100) light_value = 100;
    light_value = 100 - light_value;  



    // ??????
    index = (index + 1) % 5;

	OLED_ShowNum(5*6,0, temp,2,12);
	OLED_ShowNum(5*6,15, humi,2,12);
	OLED_ShowNum(5*6,30, light_value,2, 12);
	OLED_ShowNum(5*6,45, smoke_value, 2, 12);
	OLED_ShowNum(60+5*6, 15, co_value, 2, 12);
	OLED_Refresh_Gram();
	printf("温度= %d%% 湿度= %d%% 光照强度=%d%% 烟雾浓度=%d%% CO浓度=%d%%\r\n",temp,humi,light_value, smoke_value, co_value);

 //将值传递给全局变量
	temp_id=temp;
	humi_id=humi;                              
	smoke_value_id=smoke_value;
	light_value_id=light_value;
	co_value_id=co_value;
}

//蜂鸣器检测报警
void beep_chek(){

	//目前是烟雾检测和co浓度检测提示报警
	  if (smoke_value_id > smoke_se || co_value_id> co_set) {
        sta = 1;//标志位        
        BEEP_Alarm(125, 254);  
    } else {
        sta = 0;    
    }
		
};
	
int retry= 10;//检测倒计时，勿动
	
int main()
{
	u8 i=0;
	u16 value=0;
	u8 buf[3];
	u8 key=0;
	
	SysTick_Init(72);
	LED_Init();
	KEY_Init();
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组分2组
	USART1_Init(115200);
	LED_Init();
	ADCx_Init();
	BEEP_Init();
	OLED_Init();
	SMG_Init();
	WiFi_Config();//wifi配置

	ESP8266_Choose (ENABLE);//启动esp8266
	
	//ESP8266_AT_Test();//at测试

		
	//检测DHT11是否存在
	OLED_ShowString(0,0,"DHT11 Init...",12);
	OLED_Refresh_Gram();
	while(DHT11_Init()&& retry--)	
	{
		printf("DHT11 Check Error!\r\n",10-retry);
		OLED_ShowString(0,0,"DHT11 Check Error!",12);
		OLED_Refresh_Gram();
		delay_ms(500);
		if(retry==0){
		printf("DTH11 Init Failed.\r\n");
		OLED_ShowString(0, 15, "Init Failed!", 12);
    return 1;  
	}		
	};
	printf("DHT11 Check OK!\r\n");
	
	BEEP_Alarm(125,254);//响一声 初始化完成
	
	OLED_Clear();
	//oled显示屏
	OLED_ShowString(0, 0,  "Temp:  C",12);
	OLED_ShowString(0, 15, "Humi:  %",12);
	OLED_ShowString(0, 30, "Ligh:  %", 12); 
	OLED_ShowString(0, 45, "Smok:  %", 12);
	OLED_ShowString(60, 15, "CO:   %",12);  
	OLED_Refresh_Gram();
	setup_wifi_and_mqtt();//连接wifi和云服务器

	
	while(1)
	{	
		i++;
		//一个好看的灯
		if(i%10==0)
			LED0=!LED0;
		
			if(i%20==0)
		{//数据处理
			LED1=!LED1;
			data_pros();
				}
		
	    if(i%30==0)
		{//报警检测
			beep_chek();
			}
		
		key=KEY_Scan(0);   //扫描按键
		switch(key)
		{
			case KEY1_PRESS: LED5=!LED5;;break;    //D5指示灯
			case KEY2_PRESS: LED6=!LED6;break;    //D6指示灯
			case KEY3_PRESS: LED7=!LED7;key_id=1;break;    //D7指示灯
			case KEY4_PRESS: LED8=!LED8;key_id=0;break;    //D8指示灯
		}
	
		//	printf("CO_ADC=%d, Smoke_ADC=%d\n", 
     //  Get_Stable_ADC_Value(ADC_Channel_7), 
    //   Get_Stable_ADC_Value(ADC_Channel_8));
		if(i%50==0)
		{	//向云平台发送数据		   
			send_properties_report(temp_id,humi_id,light_value_id,smoke_value_id, co_value_id,key_id);		
			}

		
		delay_ms(10);
	}
}
