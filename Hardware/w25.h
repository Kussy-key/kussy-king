#ifndef __W25Q64_H
#define __W25Q64_H

#include "stm32f10x.h"


#define CS_ENABLE     GPIO_ResetBits(GPIOA, GPIO_Pin_4)  
#define CS_DISENABLE  GPIO_SetBits(GPIOA, GPIO_Pin_4)   

#define W25Q64_OK            0 
#define W25Q64_ERR_TIMEOUT   1    


#define W25Q64_BUSY_TIMEOUT  200000u


void    W25Q64_Init(void);
uint8_t W25Q64_WaitBusy(void);                           
uint8_t W25Q64_Enable(void);                           
uint8_t W25Q64_Erase64K(uint8_t blockNB);                 
uint8_t W25Q64_Erase4K(uint32_t addr);                       
uint8_t W25Q64_PageWrite(uint8_t *wbuff, uint16_t pageNB);   
uint8_t W25Q64_Read(uint8_t *rbuff, uint32_t addr, uint16_t datalen); 
uint8_t W25Q64_ReadID(uint32_t *id);                        

#endif
