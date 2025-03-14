
/*
�������ڲ��ԣ����������ϴ����� ���Դ���   ��ʹ���ڴ�������ֱ��  esp8266��
AT+MQTTPUB=0,"$oc/devices/{device_id}/sys/properties/report","{\"services\":[{\"service_id\":\"Smoge\"\,\"properties\":{\"Light\":10\,\"Temp\":50\,\"Hum\":60\,\"CO\":10\,\"Smo\":5}}]}",1,0

1.����������������esp8266�ʹ���������ͬʱʹ�ã�pb11�޸���pa8
2.�������ӻ�Ϊ��ƽ̨����Ȩ����ͨ�������ķ����������⣬ƽ̨�޷�������Ϣ(�ѽ�������ϸ���ת���ַ�)    
3.temp,humi,light_value,smoke_value,co_value,�����ֵ����˴���������
4.wifi_config.c���޸Ĳ����ʣ�Ĭ��115200

���£���Ҫʹ�øô��룬�����޸����е�mtqq��Ӧ�����ã��Լ�wifi�ȵ㣬��ο���Ϊ�ƣ�����ʾ��Ī����



dht��ͨ����pb11�޸���pa8
�����adת��ͨ���޸���pb0��Ҳ����ͨ��8



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

// MQTT����
#define MQTT_BROKER          												  "��ַ"
#define MQTT_IP              												  "ͬ�ϣ�ping����"
#define MQTT_PORT            									        "1883"
#define MQTT_CLIENT_ID 																"�Լ�����ƽ̨"
#define MQTT_USERNAME 															  "�Լ�����ƽ̨"
#define MQTT_PASSWORD  																"�Լ�����ƽ̨"


typedef unsigned int u32;
char at_cmd[1024] = {0};

#define ADC_SAMPLES 20
//��������ط���Ҫ�޸ģ������������
    const char *ssid     = 		"114514";    													//wifi����
    const char *wifi_pwd = 		"123457890"; 													//wifi����
		const char *topic    = 		"$oc/devices/{device_id}/sys/properties/report";        
	//const char *json_payload = "{\\\"services\\\":[{\\\"service_id\\\":\\\"Smoge\\\"\\,\\\"properties\\\":{\\\"Light\\\":10\\,\\\"Temp\\\":50\\,\\\"Hum\\\":60\\,\\\"CO\\\":10\\,\\\"Smg\\\":5}}]}";
		char json_payload[256];

//ȫ�����Զ���
	u8 temp_id=1;  	    
	u8 humi_id=1;
	u16 light_value_id=1.0;
	u16 smoke_value_id=1.0, co_value_id=1.0;
	u8 key_id=0;

		u16 smoke_se = 50;  
		u16 co_set = 5; 
    u8 sta = 0;   

//��������  ������ƽ̨���ͣ���Ҫ�޸����ϸ��ո�ʽ�޸�    const char *topic = "$oc/devices/{device����id}/sys/properties/report";
/*
		���ĵ�ַdivce_id��Ҫ�������ߵ�{}  ���ٷ����Լ�������̳�û���ᵽ����һ��Ҫ���ϣ�ͨ��c����������atָ��ת��ü�\\\

 snprintf(json_payload, sizeof(json_payload),
        "{\\\"services\\\":[{\\\"service_id\\\":\\\"Smoge\\\"\\,\\\"properties\\\":{"
        "\\\"Light\\\":%d\\,"     
        "\\\"Temp\\\":%d\\,"    
        "\\\"Hum\\\":%d\\,"     
        "\\\"CO\\\":%d\\,"        
        "\\\"Smg\\\":%d"          
        "}}]}",
        light_value, temp, humi, co_value, smoke_value);
���ֻ���޸Ķ�Ӧ�������磺Light�ȣ����ϸ�����϶�Ӧ��
*/

void send_properties_report(u8 temp, u8 humi, u16 light_value, u16 smoke_value, u16 co_value,u8 key_id)
{

   const char *topic = "$oc/devices/{device����id}/sys/properties/report";
   
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
		
		//�ϴ��ж�
if (ESP8266_Cmd(at_cmd, "OK", "ERROR", 5000)) {
    printf("MQTT Publish Success!\n");
} else {
    printf("MQTT Publish Failed!\n");
}
			
}


//ǰ5��û���⣬���涩��Ӧ���Ǹ�ʽ���⣨���޶���Ŀǰ����ʹ�ã�
void setup_wifi_and_mqtt(void)
{  
    // 1. ESP8266  STA���� ���ö���
    snprintf(at_cmd, sizeof(at_cmd), "AT+CWMODE=1");
    if (ESP8266_Cmd(at_cmd, "OK", "ERROR", 3000))
    {
        printf("STA���óɹ�\n");
    }
    else
    {
        printf("STA����ʧ��\n");
        return;
    }
    delay_ms(500); 

    // 2. WiFi���ӣ������������޸�
    snprintf(at_cmd, sizeof(at_cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, wifi_pwd);
    if (ESP8266_Cmd(at_cmd, "WIFI CONNECTED", "ERROR", 10000))
    {
        printf("WiFi���ӳɹ�\n");
    }
    else
    {
        printf("WiFi����ʧ��\n");
        return;
    }
    delay_ms(1000);  // 

    // 3. MQTT�����û�
    // �����û�:AT+MQTTUSERCFG=0,1,"NULL","�û���","����",0,0,""
		snprintf(at_cmd, sizeof(at_cmd), "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"", MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
    if (ESP8266_Cmd(at_cmd, "OK", "ERROR", 3000))
    {
        printf("MQTT�����û��ɹ�\n");
    }
    else
    {
        printf("MQTT�����û�ʧ��\n");
        return;
    }
    delay_ms(1000);

    // 4. MQTT����ClientID
    // ����:AT+MQTTCLIENTID=0,"ClientID"
    snprintf(at_cmd, sizeof(at_cmd), "AT+MQTTCLIENTID=0,\"%s\"", MQTT_CLIENT_ID);
    if (ESP8266_Cmd(at_cmd, "OK", "ERROR", 3000))
    {
        printf("MQTT ClientID�ɹ�\n");
    }
    else
    {
        printf("MQTT ClientIDʧ��\n");
        return;
    }
    delay_ms(1000);
		//mqtt����                                                          ��ip�Ͷ˿�
    snprintf(at_cmd, sizeof(at_cmd), "AT+MQTTCONN=0,\"%s\",%s,1", MQTT_BROKER, MQTT_PORT);
    if (ESP8266_Cmd(at_cmd, "OK", "ERROR", 5000))
    {
        printf("MQTT���ӳɹ�\n");
    }
    else
    {
        printf("MQTT����ʧ��\n");
        return;
    }
    delay_ms(1000);  // 

		
//ת��mqtt����
    snprintf(at_cmd, sizeof(at_cmd), "AT+MQTTSUB=0,\"%s\",1", topic);
    if (ESP8266_Cmd(at_cmd, "OK", "ERROR", 3000))
    {
        printf("MQTT�ɹ�����\n");
    }
    else
    {
        printf("MQTTʧ�ܶ���\n");
        return;
    }
    delay_ms(1000);
		BEEP_Alarm(125,254);//��һ�� ��ʼ�����
		
}
    u8 temp, humi;//������ʪ��
    u16 light_value;//�����ǿ
    u16 smoke_value, co_value;//���������coŨ��
       
    static u16 co_history[5] = {0};  
    static u16 smoke_history[5] = {0};
    static u8 index = 0;              
		u16 smoke_raw;
		u16 co_raw;

//���ݴ���
void data_pros() 
{

  //dht11��ȡ��ʪ��
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
  //��ǿŨ��ת��
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
	printf("�¶�= %d%% ʪ��= %d%% ����ǿ��=%d%% ����Ũ��=%d%% COŨ��=%d%%\r\n",temp,humi,light_value, smoke_value, co_value);

 //��ֵ���ݸ�ȫ�ֱ���
	temp_id=temp;
	humi_id=humi;                              
	smoke_value_id=smoke_value;
	light_value_id=light_value;
	co_value_id=co_value;
}

//��������ⱨ��
void beep_chek(){

	//Ŀǰ���������coŨ�ȼ����ʾ����
	  if (smoke_value_id > smoke_se || co_value_id> co_set) {
        sta = 1;//��־λ        
        BEEP_Alarm(125, 254);  
    } else {
        sta = 0;    
    }
		
};
	
int retry= 10;//��⵹��ʱ����
	
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
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //�ж����ȼ������2��
	USART1_Init(115200);
	LED_Init();
	ADCx_Init();
	BEEP_Init();
	OLED_Init();
	SMG_Init();
	WiFi_Config();//wifi����

	ESP8266_Choose (ENABLE);//����esp8266
	
	//ESP8266_AT_Test();//at����

		
	//���DHT11�Ƿ����
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
	
	BEEP_Alarm(125,254);//��һ�� ��ʼ�����
	
	OLED_Clear();
	//oled��ʾ��
	OLED_ShowString(0, 0,  "Temp:  C",12);
	OLED_ShowString(0, 15, "Humi:  %",12);
	OLED_ShowString(0, 30, "Ligh:  %", 12); 
	OLED_ShowString(0, 45, "Smok:  %", 12);
	OLED_ShowString(60, 15, "CO:   %",12);  
	OLED_Refresh_Gram();
	setup_wifi_and_mqtt();//����wifi���Ʒ�����

	
	while(1)
	{	
		i++;
		//һ���ÿ��ĵ�
		if(i%10==0)
			LED0=!LED0;
		
			if(i%20==0)
		{//���ݴ���
			LED1=!LED1;
			data_pros();
				}
		
	    if(i%30==0)
		{//�������
			beep_chek();
			}
		
		key=KEY_Scan(0);   //ɨ�谴��
		switch(key)
		{
			case KEY1_PRESS: LED5=!LED5;;break;    //D5ָʾ��
			case KEY2_PRESS: LED6=!LED6;break;    //D6ָʾ��
			case KEY3_PRESS: LED7=!LED7;key_id=1;break;    //D7ָʾ��
			case KEY4_PRESS: LED8=!LED8;key_id=0;break;    //D8ָʾ��
		}
	
		//	printf("CO_ADC=%d, Smoke_ADC=%d\n", 
     //  Get_Stable_ADC_Value(ADC_Channel_7), 
    //   Get_Stable_ADC_Value(ADC_Channel_8));
		if(i%50==0)
		{	//����ƽ̨��������		   
			send_properties_report(temp_id,humi_id,light_value_id,smoke_value_id, co_value_id,key_id);		
			}

		
		delay_ms(10);
	}
}
