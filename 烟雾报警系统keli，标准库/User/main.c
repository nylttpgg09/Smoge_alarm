
/*
经过串口测试，可以正常上传属性 测试代码   （使用于串口助手直插  esp8266）
AT+MQTTPUB=0,"$oc/devices/{device_id}/sys/properties/report","{\"services\":[{\"service_id\":\"Smoge\"\,\"properties\":{\"Fire\":10\,\"Temp\":50\,\"Hum\":60\,\"Smo\":5}}]}",1,0

1.传感器工作正常，esp8266和传感器可以同时使用，pb11修改至pa8
2.可以连接华为云平台，鉴权可以通过，订阅发布存在问题，平台无法接收消息(已解决，请严格按照转义字符)    
3.temp,humi,fire_value,smoke_value 值存放了传感器数据
4.wifi_config.c里修改波特率，默认115200

最新，若要使用该代码，请先修改下列的mtqq对应的配置，以及wifi热点，请参考华为云，会提示怎莫设置




dht的通道由pb11修改至pa8
烟雾的ad转换通道修改至pb1，也就是通道9


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
#include <math.h>

#define LIGHT_X 5*6    
#define LIGHT_Y 30   

// MQTT配置
#define MQTT_BROKER          												  ""
#define MQTT_IP              												  ""
#define MQTT_PORT            									        "1883"
#define MQTT_CLIENT_ID 																""
#define MQTT_USERNAME 															  ""
#define MQTT_PASSWORD  																""


typedef unsigned int u32;
char at_cmd[1024] = {0};

#define ADC_SAMPLES 20
//最好两个地方都要修改，避免出现问题
    const char *ssid     = 		"114514";    													//wifi名称
    const char *wifi_pwd = 		"123457890"; 													//wifi密码
		const char *topic    = 		"$oc/devices/{}/sys/properties/report";        
	//const char *json_payload = "{\\\"services\\\":[{\\\"service_id\\\":\\\"Smoge\\\"\\,\\\"properties\\\":{\\\"Light\\\":10\\,\\\"Temp\\\":50\\,\\\"Hum\\\":60\\,\\\"CO\\\":10\\,\\\"Smg\\\":5}}]}";
		char json_payload[256];

/* MQ-2 (smoge)  MQ-5 (fire)  *****/

#define RL_MQ2 5.0f
#define RL_MQ5 5.0f

/* ?????????,????????? R0 ? */
static float R0_MQ2 = 10.0f; // MQ-2
static float R0_MQ5 = 10.0f; // MQ-5

/* MQ-2 
   log(Rs/R0) = A * log(C) + B
   (C = ppm)
   ?? A,B ??????? MQ-2 ??????? */
static float A_MQ2 = -0.45f;
static float B_MQ2 = 2.30f;



static float A_MQ5 = -0.38f;
static float B_MQ5 = 1.98f;


static float M_MQ2 = 16.0f; 
static float M_MQ5 = 16.0f; // ??

//全局属性定义
	u8 temp_id=1;  	    
	u8 humi_id=1;
	u16 fire_value_id=1.0;
	u16 smoke_value_id=1.0;
	u8 key_id=0;

		u16 smoke_set = 2500;  
		u16 fire_set = 50; 
    u8 sta = 0;   

//发送数据  ，向云平台发送，若要修改请严格按照格式修改    const char *topic = "$oc/devices/{}/sys/properties/report";
/*
		订阅地址divce_id需要包含两边的{}  ，官方的以及大多数教程没有提到，请一定要加上，通过c代码来发送at指令转义得加\\\

 snprintf(json_payload, sizeof(json_payload),
        "{\\\"services\\\":[{\\\"service_id\\\":\\\"Smoge\\\"\\,\\\"properties\\\":{"  
        "\\\"Temp\\\":%d\\,"    
        "\\\"Hum\\\":%d\\,"     
        "\\\"Fire\\\":%d\\,"        
        "\\\"Smg\\\":%d"          
        "}}]}",
        fire_value, temp, humi, smoke_value);
这个只用修改对应的属性如：Light等，请严格和以上对应。
*/

void send_properties_report(u8 temp_id, u8 humi_id, u16 fire_value_id, u16 smoke_value_id,u8 key_id)
{

   const char *topic = "$oc/devices/{}/sys/properties/report";
   
    snprintf(json_payload, sizeof(json_payload),
        "{\\\"services\\\":[{\\\"service_id\\\":\\\"Smoge\\\"\\,\\\"properties\\\":{"
        "\\\"Fire\\\":%d\\,"     
        "\\\"Temp\\\":%d\\,"    
        "\\\"Hum\\\":%d\\,"     
				"\\\"Fee\\\":%d\\,"     		
        "\\\"Smg\\\":%d"          
        "}}]}",
        fire_value_id, temp_id, humi_id,key_id, smoke_value_id);

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

float MQ_GetRs(u16 adc_value, float rl_kohm)
{			float rs;
	 float vout;

    vout = (adc_value / 4095.0f) * 3.3f;
    if (vout < 0.001f) { // ?????
        vout = 0.001f;
    }
     rs = rl_kohm * (3.3f - vout) / vout;
    return rs;
}

void MQ2_Calibrate(void)
{

    u32 sum = 0;
    const int CAL_TIMES = 20;
u16 avg_adc;
	float rs;
	int i;
    for( i=0; i<CAL_TIMES; i++){
        u16 adc_raw = Get_ADC_Value(ADC_Channel_9, 10);
        sum += adc_raw;
        delay_ms(50);
    }
    avg_adc = sum / CAL_TIMES;

    // Rs
     rs = MQ_GetRs(avg_adc, RL_MQ2);

    //  datasheet ??: ? clean air ? Rs/R0 ~ 9.8 (??)
    // => R0 = Rs / 9.8
    R0_MQ2 = rs / 9.8f;
    printf("MQ-2 Calibrate Done, R0_MQ2 = %.2f kOhm\r\n", R0_MQ2);
}

void MQ5_Calibrate(void)
{
    // ,MQ-5 ADC_Channel_9
    u32 sum = 0;
    const int CAL_TIMES = 20;
 u16 avg_adc ;
	int i;
    float rs;
    for( i=0; i<CAL_TIMES; i++){
        u16 adc_raw = Get_ADC_Value(ADC_Channel_7, 10);
        sum += adc_raw;
        delay_ms(50);
    }
     avg_adc = sum / CAL_TIMES;
     rs = MQ_GetRs(avg_adc, RL_MQ5);

    //  datasheet ???:clean air ? Rs/R0 ~ 6.5()
    // => R0 = Rs / 6.5
    R0_MQ5 = rs / 6.5f;
    printf("MQ-5 Calibrate Done, R0_MQ5 = %.2f kOhm\r\n", R0_MQ5);
}

// 
float MQ2_ReadPPM(void)
{ float logC;
	float ppm;
	float ratio;
    // ??? ADC(???? data_pros ???? smoke_raw)
    u16 raw = Get_ADC_Value(ADC_Channel_9, 10);
    float rs = MQ_GetRs(raw, RL_MQ2);
    
    ratio = rs / R0_MQ2;
    // datasheet: log(Rs/R0) = A * log(ppm) + B
    // => log(ppm) = (log(Rs/R0) - B)/A
     logC = (log10f(ratio) - B_MQ2) / A_MQ2;
     ppm = powf(10, logC);
    return ppm;
}

float MQ2_ReadMgPerM3(void)
{
    float ppm = MQ2_ReadPPM();
    // mg/m^3 ~ ppm * (M / 24.45)
    float mg_m3 = ppm * (M_MQ2 / 24.45f);
    return mg_m3;
}


// ??,MQ-5
float MQ5_ReadPPM(void)
{
    u16 raw = Get_ADC_Value(ADC_Channel_7, 10);
    float rs = MQ_GetRs(raw, RL_MQ5);

    float ratio = rs / R0_MQ5;
    float logC = (log10f(ratio) - B_MQ5) / A_MQ5;
    float ppm = powf(10, logC);
    return ppm;
}

float MQ5_ReadMgPerM3(void)
{
    float ppm = MQ5_ReadPPM();
    float mg_m3 = ppm * (M_MQ5 / 24.45f);
    return mg_m3;
}



//前5步没问题，后面订阅应该是格式问题（已修订，目前均可使用）
u8 setup_wifi_and_mqtt(void)
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
        return 1;
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
        return 1;
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
        return 1;
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
        return 1;
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
        return 1;
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
        return 1;
    }
    delay_ms(1000);
		BEEP_Alarm(125,254);//响一声 初始化完成
		return 0;
}

#define MAX_HISTORY 5  // 
static u16 smoke_history[MAX_HISTORY] = {0};
static u16 fire_history[MAX_HISTORY] = {0};
static u8 smoke_index = 0;
static u8 fire_index  = 0;
    u8 temp, humi;//定义温湿度
    u16 fire_value;//定义天然气浓度
    u16 smoke_value;//定义烟雾
		u16 smoke_raw;
		u16 fire_raw;
u16 smooth_value(u16 *history, u8 *index, u16 new_value) {

		u8 ii = 0;
    u32 sum = 0;
    history[*index] = new_value;
    *index = (*index + 1) % MAX_HISTORY;
    

    for (ii = 0; ii < MAX_HISTORY; ii++) {
        sum += history[ii];
    }
    return sum / MAX_HISTORY;
}
//数据处理
void data_pros() 
{
	//dht11读取温湿度

    // 2. ?????? (MQ-2)
    float mq2_ppm = MQ2_ReadPPM(); 

    float mq5_ppm = MQ5_ReadPPM();
    
    int smoke_ppm  = (int)mq2_ppm;
    int fire_ppm   = (int)mq5_ppm;
    if (smoke_ppm < 0) smoke_ppm = 0;
    if (fire_ppm  < 0) fire_ppm  = 0;
    
   DHT11_Read_Data(&temp, &humi);
	OLED_ShowNum(5*6,0, temp,2,12);
	OLED_ShowNum(5*6,15, humi,2,12);
	OLED_ShowNum(5*6,30, fire_ppm,5, 12);
	OLED_ShowNum(5*6,45, smoke_ppm, 5, 12);
	OLED_Refresh_Gram();
	printf("温度= %d%% 湿度= %d%% 天然气浓度=%d ppm 烟雾浓度=%d  ppm \r\n",temp,humi, fire_ppm, smoke_ppm);

 //将值传递给全局变量
		temp_id        = temp;
    humi_id        = humi;
    fire_value_id  = fire_ppm;   // 
    smoke_value_id = smoke_ppm;
    
}

//蜂鸣器检测报警
void beep_chek(){

	//目前是烟雾检测和co浓度检测提示报警
	  if (smoke_value_id > smoke_set || fire_value_id> fire_set) {
        sta = 1;//标志位        
        BEEP_Alarm(125, 254);  
    } else {
        sta = 0;    
    }
		
};
	
int retry= 10;//检测倒计时，勿动
int	retry_wifi=10;
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
MQ2_Calibrate();  // ?? MQ-2
MQ5_Calibrate();  // ?? MQ-5
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
	OLED_ShowString(0, 30, "Fire:   ppm", 12); 
	OLED_ShowString(0, 45, "Smok:   ppm", 12);
	OLED_Refresh_Gram();
	while(setup_wifi_and_mqtt() && retry_wifi--)
	{
		printf("WIFI and mqtt Error!\r\n",10-retry);
		OLED_ShowString(0,0,"WIFI mqtt Check Error!",12);
		OLED_Refresh_Gram();
		delay_ms(500);
		if(retry==0){
		printf("WIFI and mqtt Init Failed.\r\n");
		OLED_ShowString(0, 15, "Init Failed!", 12);
    return 1;  
	}
	};//连接wifi和云服务器
	
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
			case KEY1_PRESS: LED5=!LED5;break;    //D5指示灯
			case KEY2_PRESS: LED6=!LED6;break;    //D6指示灯
			case KEY3_PRESS: LED7=!LED7;key_id=1;break;    //D7指示灯
			case KEY4_PRESS: LED8=!LED8;key_id=0;break;    //D8指示灯
		}
	
	

		if(i%50==0)
		{	//向云平台发送数据		   
			send_properties_report(temp_id,humi_id,fire_value_id,smoke_value_id, key_id);		
			}

		
		delay_ms(10);
	}
}
