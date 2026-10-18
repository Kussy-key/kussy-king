#include "stm32f10x.h"             
#include "spi.h"
#include "w25.h"
/**
W25Q64初始化
 */

void W25Q64_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    CS_DISENABLE;     //拉高片选
    SPI1_Init();
}

/**
忙判断/等待
 */
uint8_t W25Q64_WaitBusy(void)
{
    uint32_t timeout = W25Q64_BUSY_TIMEOUT;
    uint8_t  res;

    do{
        CS_ENABLE;
        SPI1_ReadWriteByte(0x05);        
        res = SPI1_ReadWriteByte(0xff);
        CS_DISENABLE;
        timeout--;
    }while(((res & 0x01) == 0x01) && (timeout != 0));   //bit0位是busy位，判断是否为1

    if(timeout == 0)
        return W25Q64_ERR_TIMEOUT;
    return W25Q64_OK;
}

/**
写使能
 */
uint8_t W25Q64_Enable(void)
{
    uint8_t ret;

    ret = W25Q64_WaitBusy();
    if(ret != W25Q64_OK)
        return ret;

    CS_ENABLE;
    SPI1_ReadWriteByte(0x06);           
    CS_DISENABLE;
    return W25Q64_OK;
}

/**
读ID，用于测试
 */
uint8_t W25Q64_ReadID(uint32_t *id)
{
    uint8_t ret;

    *id = 0;

    ret = W25Q64_WaitBusy();
    if(ret != W25Q64_OK)
        return ret;

    CS_ENABLE;
    SPI1_ReadWriteByte(0x9F);              /* JEDEC ID */
    *id  = (uint32_t)SPI1_ReadWriteByte(0xff) << 16;   /* Manufacturer: 0xEF */
    *id |= (uint32_t)SPI1_ReadWriteByte(0xff) << 8;    /* Memory Type:  0x40 */
    *id |= (uint32_t)SPI1_ReadWriteByte(0xff);         /* Capacity:     0x17 */
    CS_DISENABLE;
    return W25Q64_OK;
}
/**
64KB 块擦除
blockNB 块号 0~127
 */
uint8_t W25Q64_Erase64K(uint8_t blockNB)
{
    uint8_t wdata[4];
    uint8_t ret;

    wdata[0] = 0xD8;                       //Block Erase 64KB 
    wdata[1] = (blockNB * 64 * 1024) >> 16;
    wdata[2] = (blockNB * 64 * 1024) >> 8;
    wdata[3] = (blockNB * 64 * 1024) >> 0;

    ret = W25Q64_Enable();                //擦除前先 Write Enable
    if(ret != W25Q64_OK)
        return ret;

    CS_ENABLE;
    SPI1_Write(wdata, 4);
    CS_DISENABLE;

    return W25Q64_WaitBusy();           
}

/**
4KB 扇区擦除
addr 扇区首地址（须 4096 对齐，如 0x7FF000）
 */
uint8_t W25Q64_Erase4K(uint32_t addr)
{
    uint8_t wdata[4];
    uint8_t ret;

    wdata[0] = 0x20;   //擦除扇区命令                   
    wdata[1] = addr >> 16;   //24位地址
    wdata[2] = addr >> 8;
    wdata[3] = addr;

    ret = W25Q64_Enable();
    if(ret != W25Q64_OK)
        return ret;

    CS_ENABLE;
    SPI1_Write(wdata, 4);
    CS_DISENABLE;

    return W25Q64_WaitBusy();           
}

/**
页编程（一次写满一页 256 字节，页内不能跨页）
wbuff 数据缓冲区（至少256字节）
pageNB 页号 0~32767
 */
uint8_t W25Q64_PageWrite(uint8_t *wbuff, uint16_t pageNB)
{
    uint8_t wdata[4];
    uint8_t ret;

    wdata[0] = 0x02;                     //页编程命令
    wdata[1] = (pageNB * 256) >> 16;      
    wdata[2] = (pageNB * 256) >> 8;
    wdata[3] = (pageNB * 256) >> 0;

    ret = W25Q64_Enable();                 
    if(ret != W25Q64_OK)
        return ret;

    CS_ENABLE;
    SPI1_Write(wdata, 4);                 
    SPI1_Write(wbuff, 256);             
    CS_DISENABLE;

    return W25Q64_WaitBusy();              
}

/**
读数据
读出缓冲区
addr    起始地址（0~0x7FFFFF）
datalen 读取长度（最大 65535，一次连续读不超过 64KB）
 */
uint8_t W25Q64_Read(uint8_t *rbuff, uint32_t addr, uint16_t datalen)
{
    uint8_t wdata[4];
    uint8_t ret;

    wdata[0] = 0x03;                      
    wdata[1] = addr >> 16;
    wdata[2] = addr >> 8;
    wdata[3] = addr;

    ret = W25Q64_WaitBusy();
    if(ret != W25Q64_OK)
        return ret;

    CS_ENABLE;
    SPI1_Write(wdata, 4);               
    SPI1_Read(rbuff, datalen);           
    CS_DISENABLE;
    return W25Q64_OK;
}




