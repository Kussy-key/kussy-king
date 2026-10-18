#ifndef __BOOT_H
#define __BOOT_H

#include <stdint.h>


//App 复位函数指针类型：接收一个占位参数（复位向量不接收参数，传0）
typedef void (*load_a)(uint32_t);

void BootLoader_Branch(void);                              
uint8_t BootLoader_Enter(uint8_t timeout);                 
void BootLoader_Info(void);                               
void BootLoader_Event(uint8_t *data, uint16_t datalen);  
void W25Q64_ReadOTAInfo(void);                           
void W25Q64_WriteOTAInfo(void);                           
__asm void MSR_SP(uint32_t addr);                     
void BootLoader_Clear(void);                             
void LOAD_A(uint32_t addr);                                
uint16_t Xmodem_CRC16(uint8_t *data, uint16_t datalen);   


#endif
