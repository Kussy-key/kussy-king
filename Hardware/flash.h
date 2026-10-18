#ifndef __FLASH_H
#define __FLASH_H

/* ============ STM32 内部 Flash 驱动对外接口（OTA 用） ============ */
void STM32_EraseFlash(uint16_t start, uint16_t num);                    //擦除start页起num个页
void STM32_WriteFlash(uint32_t saddr, uint32_t *wdata, uint32_t wnum);  //从saddr起写wnum字节（4对齐、先擦后写）


#endif
