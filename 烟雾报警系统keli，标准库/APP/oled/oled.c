#include "oled.h"
#include "iic.h"
#include "oledfont.h" 


//OLED的显存
//存放格式如下.
//[0]0 1 2 3 ... 127	
//[1]0 1 2 3 ... 127	
//[2]0 1 2 3 ... 127	
//[3]0 1 2 3 ... 127	
//[4]0 1 2 3 ... 127	
//[5]0 1 2 3 ... 127	
//[6]0 1 2 3 ... 127	
//[7]0 1 2 3 ... 127 		   
u8 OLED_GRAM[128][8];


//IIC写命令
void write_iic_cmd(u8 cmd)
{
	IIC_Start();
	IIC_Send_Byte(0x78);//Slave address,SA0=0
	IIC_Wait_Ack();	
	IIC_Send_Byte(0x00);//write command
	IIC_Wait_Ack();	
	IIC_Send_Byte(cmd); 
	IIC_Wait_Ack();	
	IIC_Stop();
}

//IIC写数据
void write_iic_data(u8 dat)
{
	IIC_Start();
	IIC_Send_Byte(0x78);//Slave address,SA0=0
	IIC_Wait_Ack();	
	IIC_Send_Byte(0x40);//write data
	IIC_Wait_Ack();	
	IIC_Send_Byte(dat); 
	IIC_Wait_Ack();	
	IIC_Stop();
}

//向SSD1306写入一个字节。
//dat:要写入的数据/命令
//cmd:数据/命令标志 0,表示命令;1,表示数据;
void OLED_WR_Byte(u8 dat,u8 cmd)
{
	if(cmd)
		write_iic_data(dat);
	else
		write_iic_cmd(dat);
}

//更新显存到LCD		 
void OLED_Refresh_Gram(void)
{
	int i,n;		    
	for(i=7;i>=0;i--) //修改刷新方向 
	{  
		OLED_WR_Byte (0xb0+i,OLED_CMD);    //设置页地址（0~7）
		OLED_WR_Byte (0x00,OLED_CMD);      //设置显示位置—列低地址
		OLED_WR_Byte (0x10,OLED_CMD);      //设置显示位置—列高地址   
		for(n=0;n<128;n++)OLED_WR_Byte(OLED_GRAM[n][i],OLED_DATA); 
	}   
}

void OLED_Set_Pos(unsigned char x, unsigned char y) 
{ 
	OLED_WR_Byte(0xb0+y,OLED_CMD);
	OLED_WR_Byte(((x&0xf0)>>4)|0x10,OLED_CMD);
	OLED_WR_Byte((x&0x0f)|0x01,OLED_CMD); 
}   	  
//开启OLED显示    
void OLED_Display_On(void)
{
	OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X14,OLED_CMD);  //DCDC ON
	OLED_WR_Byte(0XAF,OLED_CMD);  //DISPLAY ON
}
//关闭OLED显示     
void OLED_Display_Off(void)
{
	OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X10,OLED_CMD);  //DCDC OFF
	OLED_WR_Byte(0XAE,OLED_CMD);  //DISPLAY OFF
}
		   			 
//清屏函数,清完屏,整个屏幕是黑色的!和没点亮一样!!!	  
void OLED_Clear(void)  
{  
	u8 i,n;		    
	for(i=0;i<8;i++)  
	{    
		for(n=0;n<128;n++)
		{
			OLED_GRAM[n][i]=0;
		}
	}
	OLED_Refresh_Gram();//更新显示 
}

//m^n函数
u32 oled_pow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}

//画点 
//x:0~127
//y:0~63
//t:1 填充 0,清空				   
void OLED_DrawPoint(u8 x,u8 y,u8 t)
{
	u8 pos,bx,temp=0;
	if(x>127||y>63)return;//超出范围了.
	pos=7-y/8;
	bx=y%8;
	temp=1<<(7-bx);
	if(t)OLED_GRAM[x][pos]|=temp;
	else OLED_GRAM[x][pos]&=~temp;		    
}

//x1,y1,x2,y2 填充区域的对角坐标
//确保x1<=x2;y1<=y2 0<=x1<=127 0<=y1<=63	 	 
//dot:0,清空;1,填充	  
void OLED_Fill(u8 x1,u8 y1,u8 x2,u8 y2,u8 dot)  
{  
	u8 x,y;  
	for(x=x1;x<=x2;x++)
	{
		for(y=y1;y<=y2;y++)
		{ 	
			OLED_DrawPoint(x,y,dot);	
		}
	}													    
}

//填充矩形
//x0,y0:矩形的左上角坐标
//width,height:矩形的尺寸
//color:颜色
void OLED_Fill_rectangle(u8 x0,u8 y0,u8 width,u8 height,u8 color)
{	  							   
	if(width==0||height==0)return;//非法.	 
	OLED_Fill(x0,y0,x0+width-1,y0+height-1,color);	   	   
}

//画线
//x1,y1:起点坐标
//x2,y2:终点坐标  
void OLED_DrawLine(u8 x1, u8 y1, u8 x2, u8 y2,u8 color)
{
	u8 t; 
	char xerr=0,yerr=0,delta_x,delta_y,distance; 
	char incx,incy,uRow,uCol; 
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	if(delta_x>0)incx=1; //设置单步方向 
	else if(delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;} 
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//水平线 
	else{incy=-1;delta_y=-delta_y;} 
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y; 
	for(t=0;t<=distance+1;t++ )//画线输出 
	{  
		OLED_DrawPoint(uRow,uCol,color);//画点 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
}

//画大点函数
//x0,y0:坐标
//color:颜色
//以(x0,y0)为中心,画一个9个点的大点
void OLED_Draw_Bigpoint(u8 x0,u8 y0,u8 color)
{
	u8 i,j;
	u8 x,y;				    
	if(x0>=1)x=x0-1;
	else x=x0;
	if(y0>=1)y=y0-1;
	else y=y0;
	for(i=y;i<y0+2;i++)
	{
		for(j=x;j<x0+2;j++)OLED_DrawPoint(j,i,color);
	}  						   
}

//画垂直线
//x0,y0:坐标
//len:线长度
//color:颜色
void OLED_Draw_vline(u8 x0,u8 y0,u8 len,u8 color)
{
	if((len==0)||(x0>Max_Column-1)||(y0>Max_Row-1))return;
	OLED_Fill(x0,y0,x0,y0+len-1,color);	
}

//画水平线
//x0,y0:坐标
//len:线长度
//color:颜色
void OLED_Draw_hline(u8 x0,u8 y0,u8 len,u8 color)
{
	if((len==0)||(x0>Max_Column-1)||(y0>Max_Row-1))return;
	OLED_Fill(x0,y0,x0+len-1,y0,color);	
}

//画实心圆
//x0,y0:坐标
//r半径
//color:颜色
void OLED_Fill_circle(u8 x0,u8 y0,u8 r,u8 color)
{											  
	u16 i;
	u16 imax = ((u32)r*707)/1000+1;
	u16 sqmax = (u32)r*(u32)r+(u32)r/2;
	u16 x=r;
	OLED_Draw_hline(x0-r,y0,2*r,color);
	for (i=1;i<=imax;i++) 
	{
		if ((i*i+x*x)>sqmax) 
		{
			// draw lines from outside  
			if (x>imax) 
			{
				OLED_Draw_hline (x0-i+1,y0+x,2*(i-1),color);
				OLED_Draw_hline (x0-i+1,y0-x,2*(i-1),color);
			}
			x--;
		}
		// draw lines from inside (center)  
		OLED_Draw_hline(x0-x,y0+i,2*x,color);
		OLED_Draw_hline(x0-x,y0-i,2*x,color);
	}
}

//画椭圆
//x0,y0:坐标
//rx:x方向半径
//ry:y方向半径
void OLED_Draw_ellipse(u8 x0, u8 y0, u8 rx, u8 ry,u8 color) 
{
	u16 OutConst, Sum, SumY;
	u8 x,y;
	u8 xOld;
	u8 _rx = rx;
	u8 _ry = ry;
	if(rx>x0||ry>y0)return;//非法.
	OutConst = _rx*_rx*_ry*_ry+(_rx*_rx*_ry>>1);  // Constant as explaint above 
	// To compensate for rounding  
	xOld = x = rx;
	for (y=0; y<=ry; y++) 
	{
		if (y==ry)x=0; 
		else 
		{
			SumY =((u16)(rx*rx))*((u32)(y*y)); // Does not change in loop  
			while(Sum = SumY + ((u16)(ry*ry))*((u32)(x*x)),(x>0) && (Sum>OutConst)) x--;
		}
		// Since we draw lines, we can not draw on the first iteration		    
		if (y) 
		{
			OLED_DrawLine(x0-xOld,y0-y+1,x0-x,y0-y,color);
			OLED_DrawLine(x0-xOld,y0+y-1,x0-x,y0+y,color);
			OLED_DrawLine(x0+xOld,y0-y+1,x0+x,y0-y,color);
			OLED_DrawLine(x0+xOld,y0+y-1,x0+x,y0+y,color);
		}
		xOld = x;
	}
}

//画矩形	  
//(x1,y1),(x2,y2):矩形的对角坐标
void OLED_DrawRectangle(u8 x1, u8 y1, u8 x2, u8 y2,u8 color)
{
	OLED_DrawLine(x1,y1,x2,y1,color);
	OLED_DrawLine(x1,y1,x1,y2,color);
	OLED_DrawLine(x1,y2,x2,y2,color);
	OLED_DrawLine(x2,y1,x2,y2,color);
}

//在指定位置画一个指定大小的圆
//(x,y):中心点
//r    :半径
void OLED_Draw_Circle(u8 x0,u8 y0,u8 r,u8 color)
{
	char a,b;
	char di;
	a=0;b=r;	  
	di=3-(r<<1);             //判断下个点位置的标志
	while(a<=b)
	{
		OLED_DrawPoint(x0+a,y0-b,color);             //5
 		OLED_DrawPoint(x0+b,y0-a,color);             //0           
		OLED_DrawPoint(x0+b,y0+a,color);             //4               
		OLED_DrawPoint(x0+a,y0+b,color);             //6 
		OLED_DrawPoint(x0-a,y0+b,color);             //1       
 		OLED_DrawPoint(x0-b,y0+a,color);             
		OLED_DrawPoint(x0-a,y0-b,color);             //2             
  		OLED_DrawPoint(x0-b,y0-a,color);             //7     	         
		a++;
		//使用Bresenham算法画圆     
		if(di<0)di +=4*a+6;	  
		else
		{
			di+=10+4*(a-b);   
			b--;
		} 						    
	}
}  

//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
//mode:0,反白显示;1,正常显示				 
//size:选择字体 12/16/24
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size,u8 mode)
{      			    
	u8 temp,t,t1;
	u8 y0=y;
	u8 csize=(size/8+((size%8)?1:0))*(size/2);		//得到字体一个字符对应点阵集所占的字节数
	chr=chr-' ';//得到偏移后的值		 
    for(t=0;t<csize;t++)
    {   
		if(size==12)temp=ascii_1206[chr][t]; 	 	//调用1206字体
		else if(size==16)temp=ascii_1608[chr][t];	//调用1608字体
		else if(size==24)temp=ascii_2412[chr][t];	//调用2412字体
		else return;								//没有的字库
        for(t1=0;t1<8;t1++)
		{
			if(temp&0x80)OLED_DrawPoint(x,y,mode);
			else OLED_DrawPoint(x,y,!mode);
			temp<<=1;
			y++;
			if((y-y0)==size)
			{
				y=y0;
				x++;
				break;
			}
		}  	 
    }          
}

//显示2个数字
//x,y :起点坐标	 
//len :数字的位数
//size:字体大小
//num:数值(0~4294967295);	 		  
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size)
{         	
	u8 t,temp;
	u8 enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/oled_pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(size/2)*t,y,' ',size,1);
				continue;
			}else enshow=1; 
		 	 
		}
	 	OLED_ShowChar(x+(size/2)*t,y,temp+'0',size,1); 
	}
} 


//显示字符串
//x,y:起点坐标  
//size:字体大小 
//*p:字符串起始地址 
void OLED_ShowString(u8 x,u8 y,const u8 *p,u8 size)
{	
    while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
    {       
        if(x>(X_WIDTH-(size/2))){x=0;y+=size;}
        if(y>(Y_WIDTH-size)){y=x=0;OLED_Clear();}
        OLED_ShowChar(x,y,*p,size,1);	 
        x+=size/2;
        p++;
    }  	
}


//显示汉字
//x,y:起点坐标  
//pos:数组位置汉字显示
//size:字体大小 
//mode:0,反白显示;1,正常显示
void OLED_ShowFontHZ(u8 x,u8 y,u8 pos,u8 size,u8 mode)
{
	u8 temp,t,t1;
	u8 y0=y;  
	u8 csize=(size/8+((size%8)?1:0))*(size);//得到字体一个字符对应点阵集所占的字节数	 
	if(size!=12&&size!=16&&size!=24)return;	//不支持的size
	 
	for(t=0;t<csize;t++)
	{   												   
		if(size==12)temp=FontHzk[pos][t]; 	 	//调用1206字体
		else if(size==16)temp=FontHzk[pos][t];	//调用1608字体
		else if(size==24)temp=FontHzk[pos][t];	//调用2412字体
		else return;								//没有的字库
        for(t1=0;t1<8;t1++)
		{
			if(temp&0x80)OLED_DrawPoint(x,y,mode);
			else OLED_DrawPoint(x,y,!mode);
			temp<<=1;
			y++;
			if((y-y0)==size)
			{
				y=y0;
				x++;
				break;
			}
		}  	 
	}  	
} 

//显示BMP图片128×64
//起始点坐标(x,y),x的范围0～127，y为页的范围0～7
void OLED_DrawBMP(u8 x0, u8 y0,u8 x1, u8 y1,u8 BMP[])
{ 	
 	u16 j=0;
 	u8 x,y;
  
  	if(y1%8==0)y=y1/8;      
  	else y=y1/8+1;
	for(y=y0;y<y1;y++)
	{
		OLED_Set_Pos(x0,y);
    	for(x=x0;x<x1;x++)
	    {      
	    	OLED_WR_Byte(BMP[j++],OLED_DATA);	    	
	    }
	}
}

//OLED GPIO初始化
//OLED初始化
void OLED_Init(void)
{	  	
	IIC_Init();
	
	OLED_WR_Byte(0xAE,OLED_CMD); //关闭显示
	OLED_WR_Byte(0xD5,OLED_CMD); //设置时钟分频因子,震荡频率
	OLED_WR_Byte(80,OLED_CMD);   //[3:0],分频因子;[7:4],震荡频率
	OLED_WR_Byte(0xA8,OLED_CMD); //设置驱动路数
	OLED_WR_Byte(0X3F,OLED_CMD); //默认0X3F(1/64) 
	OLED_WR_Byte(0xD3,OLED_CMD); //设置显示偏移
	OLED_WR_Byte(0X00,OLED_CMD); //默认为0

	OLED_WR_Byte(0x40,OLED_CMD); //设置显示开始行 [5:0],行数.
													    
	OLED_WR_Byte(0x8D,OLED_CMD); //电荷泵设置
	OLED_WR_Byte(0x14,OLED_CMD); //bit2，开启/关闭
	OLED_WR_Byte(0x20,OLED_CMD); //设置内存地址模式
	OLED_WR_Byte(0x02,OLED_CMD); //[1:0],00，列地址模式;01，行地址模式;10,页地址模式;默认10;
	OLED_WR_Byte(0xA1,OLED_CMD); //段重定义设置,bit0:0,0->0;1,0->127;
	OLED_WR_Byte(0xC0,OLED_CMD); //设置COM扫描方向;bit3:0,普通模式;1,重定义模式 COM[N-1]->COM0;N:驱动路数
	OLED_WR_Byte(0xDA,OLED_CMD); //设置COM硬件引脚配置
	OLED_WR_Byte(0x12,OLED_CMD); //[5:4]配置
		 
	OLED_WR_Byte(0x81,OLED_CMD); //对比度设置
	OLED_WR_Byte(0xEF,OLED_CMD); //1~255;默认0X7F (亮度设置,越大越亮)
	OLED_WR_Byte(0xD9,OLED_CMD); //设置预充电周期
	OLED_WR_Byte(0xf1,OLED_CMD); //[3:0],PHASE 1;[7:4],PHASE 2;
	OLED_WR_Byte(0xDB,OLED_CMD); //设置VCOMH 电压倍率
	OLED_WR_Byte(0x30,OLED_CMD); //[6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc;

	OLED_WR_Byte(0xA4,OLED_CMD); //全局显示开启;bit0:1,开启;0,关闭;(白屏/黑屏)
	OLED_WR_Byte(0xA6,OLED_CMD); //设置显示方式;bit0:1,反相显示;0,正常显示	    						   
	OLED_WR_Byte(0xAF,OLED_CMD); //开启显示	

	OLED_Clear();
}
