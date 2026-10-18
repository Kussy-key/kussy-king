#include "stm32f10x.h"            
#include <stdio.h>
#include <stdarg.h>
#include <Serial.h>

/**
串口初始化；空闲中断+DMA
  */
uint8_t U1_RxBuffer[U1_RX_SIZE];
uint8_t U1_TxBuffer[U1_TX_SIZE];
io U1io;

void Serial_Init(uint32_t baud)
{

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode  =  GPIO_Mode_AF_PP;   //发送推挽
	GPIO_InitStructure.GPIO_Pin   =  GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed =  GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);				
	
	GPIO_InitStructure.GPIO_Mode =  GPIO_Mode_IN_FLOATING;   //接收浮空
	GPIO_InitStructure.GPIO_Pin  =  GPIO_Pin_10;
	//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);				
	
	USART_InitTypeDef USART_InitStructure;					
	USART_InitStructure.USART_BaudRate            = baud;				
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	
	USART_InitStructure.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;	
	USART_InitStructure.USART_Parity              = USART_Parity_No;		
	USART_InitStructure.USART_StopBits            = USART_StopBits_1;	
	USART_InitStructure.USART_WordLength          = USART_WordLength_8b;	
	USART_Init(USART1, &USART_InitStructure);				
	

	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);			//开启串口接收数据空闲中断
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);			//配置NVIC为分组2
	
	NVIC_InitTypeDef NVIC_InitStructure;				
	NVIC_InitStructure.NVIC_IRQChannel                   = USART1_IRQn;		
	NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;			
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;		
	NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;		
	NVIC_Init(&NVIC_InitStructure);	
	
	MYDMA_Init();
	U1_PTRInit();
	USART_Cmd(USART1, ENABLE);			
}
/**
DMA初始化
  */
void MYDMA_Init(void)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
	DMA_DeInit(DMA1_Channel5);
	DMA_InitTypeDef DMA_InitStructure;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&USART1->DR;   //外设基地址，DMA从串口的数据寄存器取
	DMA_InitStructure.DMA_MemoryBaseAddr     = (u32)U1_RxBuffer;   //内存地址，缓冲数组
	DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize         = U1_RX_MAX+1;
	DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;   //外设不变
	DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;   //内存+1
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel5, ENABLE);
}

/**
串口缓冲区环形数组的指针结构体初始化
  */
void U1_PTRInit(void)
{
	U1io.data_in=&U1io.data[0];
	U1io.data_out=&U1io.data[0];
	U1io.data_end=&U1io.data[NUM-1];   //0开始，长度减一
	U1io.data_in->start=U1_RxBuffer;
	U1io.ucbcounter=0;
}
/**
定义u1_printf发送函数
  */

void u1_printf(char *format, ...)
{
	uint16_t i;
  va_list listdata;
  va_start(listdata, format);
  vsprintf((char *)U1_TxBuffer, format, listdata);
	va_end(listdata);
	    for (i = 0; i < strlen((const char *)U1_TxBuffer); i++)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, U1_TxBuffer[i]);
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}
	
	
