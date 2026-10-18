#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

#define U1_RX_SIZE 2048 
#define U1_TX_SIZE 2048
#define U1_RX_MAX 256
#define NUM 10
extern uint8_t U1_RxBuffer[U1_RX_SIZE];

typedef struct{
	uint8_t *start;
	uint8_t *end;
}se;
typedef struct{
	uint16_t ucbcounter;
	se data[NUM];
	se *data_in;
	se *data_out;
	se *data_end;
}io;

void Serial_Init(uint32_t baud);
void Serial_Printf(char *format, ...);
void MYDMA_Init(void);
void U1_PTRInit(void);
void u1_printf(char *format, ...);
extern io U1io;

#endif


