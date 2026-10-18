#include "stm32f10x.h"                  // Device header
#include "flash.h"


/**
擦除内部Flash，从start页开始的num个页
每页擦除约 20~40ms，擦 44 页约 1~2 秒
 */
void STM32_EraseFlash(uint16_t start, uint16_t num)
{
    uint16_t i;

    FLASH_Unlock();
    for(i = 0; i < num; i++)
    {
        FLASH_ErasePage((0x08000000 + start * 1024) + (1024 * i));
    }
    FLASH_Lock();
}

/**
向内部Flash指定地址连续写
 */
void STM32_WriteFlash(uint32_t saddr, uint32_t *wdata, uint32_t wnum)
{
    FLASH_Unlock();
	  if (wnum % 4 != 0)  //若不为4的倍数，会造成指针越界，内存破坏
			return;
    while(wnum)
    {
        FLASH_ProgramWord(saddr, *wdata);
        wnum -= 4;               //wnum 按字节数计，每次写 4 字节 
        saddr += 4;
        wdata++;
    }
    FLASH_Lock();
}










    
