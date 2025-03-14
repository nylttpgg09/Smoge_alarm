#ifndef _key_H
#define _key_H

#include "system.h"

//管脚定义
#define KEY1_PORT 			GPIOA  
#define KEY1_PIN 			GPIO_Pin_15
#define KEY1_PORT_RCC		RCC_APB2Periph_GPIOA

#define KEY2_PORT 			GPIOA  
#define KEY2_PIN 			GPIO_Pin_14
#define KEY2_PORT_RCC		RCC_APB2Periph_GPIOA

#define KEY3_PORT 			GPIOA  
#define KEY3_PIN 			GPIO_Pin_13
#define KEY3_PORT_RCC		RCC_APB2Periph_GPIOA

#define KEY4_PORT 			GPIOA  
#define KEY4_PIN 			GPIO_Pin_12
#define KEY4_PORT_RCC		RCC_APB2Periph_GPIOA 	


//使用位操作定义
#define KEY1 	PAin(15)
#define KEY2 	PAin(14)
#define KEY3 	PAin(13)
#define KEY4 	PAin(12)

//定义各个按键值  
#define KEY1_PRESS 		1
#define KEY2_PRESS		2
#define KEY3_PRESS		3
#define KEY4_PRESS		4
 
void KEY_Init(void);
u8 KEY_Scan(u8 mode);

#endif
