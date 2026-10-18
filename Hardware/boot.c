#include "stm32f10x.h"                  // Device header
#include "Serial.h"
#include "delay.h"
#include "main.h"
#include "w25.h"
#include "flash.h"
#include <string.h>
#include "boot.h"
load_a load_A;
/**
OTA判断：更新/跳转A区
 */
void BootLoader_Branch(void)
{
	if(BootLoader_Enter(20)==0)
	{
		if(OTA_Info.OTA_flag == OTA_SET_FLAG)
		{
			u1_printf("OTA更新\r\n");
			BootStaFlag |= UPDATA_A_FLAG;    //置"更新A区"标志，主循环里执行固件搬运
			UpDataA.W25Q64_BlockNB = 0;      //固件存W25Q64的0号块
		}
		else
		{
			u1_printf("跳转A分区\r\n");
			LOAD_A(STM32_A_SADDR);           //APP区从0x08005000开始
		}
	}
	u1_printf("进入BootLoader命令行\r\n");
	BootLoader_Info();
}

/**
命令行进入函数：timeout*100ms内输入小写字母w进入命令行
 */
uint8_t BootLoader_Enter(uint8_t timeout)
{
    u1_printf("%dms内，输入小写字母w，进入BootLoader命令行\r\n", timeout*100);
    while(timeout--)
    {
        Delay_ms(100);
        if(U1_RxBuffer[0] == 'w')
        {
            return 1;                    // 进入命令行
        }
    }
    return 0;                            // 不进入命令行
}

/**
命令行事件选取函数
 */
void BootLoader_Event(uint8_t *data, uint16_t datalen)
{
	int temp, i;
	if(BootStaFlag == 0)
	{
		if((datalen == 1) && (data[0] == '1'))
		{
			u1_printf("擦除A区\r\n");
			STM32_EraseFlash(STM32_A_START_PAGE, STM32_A_PAGE_NUM);
		}
		else if((datalen == 1) && (data[0] == '2'))
		{
			u1_printf("通过Xmodem协议，串口IAP下载A区程序（bin文件）\r\n");
			STM32_EraseFlash(STM32_A_START_PAGE, STM32_A_PAGE_NUM);
			BootStaFlag |= (IAP_XMODEMC_FLAG | IAP_XMODEMD_FLAG);
			UpDataA.XmodemTimer = 0;
			UpDataA.XmodemNum = 0;
		}
		else if((datalen == 1) && (data[0] == '3'))
		{
			u1_printf("设置版本号\r\n");	
			BootStaFlag |= SET_VERSION_FLAG;
		}
		else if((datalen == 1) && (data[0] == '4'))
		{
			u1_printf("查询版本号\r\n");	
			W25Q64_ReadOTAInfo();
			u1_printf("版本号:%s\r\n",OTA_Info.OTA_ver);
			BootLoader_Info();
		}
		else if((datalen == 1) && (data[0] == '5'))
		{
			u1_printf("向外部Flash下载程序，请输入需要用到的块号（1-9）\r\n");
			BootStaFlag |= CMD_5_FLAG;
		}
		else if((datalen == 1) && (data[0] == '6'))
		{
			u1_printf("使用外部Flash内的程序，请输入需要用到的块号（1-9）\r\n");
			BootStaFlag |= CMD_6_FLAG;
		}
		else if((datalen == 1) && (data[0] == '7'))
		{
			u1_printf("重启\r\n");
			Delay_ms(100);
			NVIC_SystemReset();
		}
	}
	else if(BootStaFlag & IAP_XMODEMD_FLAG)
	{
		/* 数据包：SOH(1) + 包号(1) + 包号补码(1) + 数据(128) + CRC16(2) = 133字节 */
		if((datalen == 133) && (data[0] == 0x01))
		{
			BootStaFlag &= ~IAP_XMODEMC_FLAG;                    /* 收到首包就停发'C' */
			UpDataA.XmodemCRC = Xmodem_CRC16(&data[3], 128);    /* 只校验128字节数据段 */
			if(UpDataA.XmodemCRC == (data[131] << 8 | data[132]))
			{
				UpDataA.XmodemNum++;                             /* CRC通过，包号+1 */
				/* 包在1KB页缓冲里的偏移：每页装8个128B包，取模算槽位 */
				memcpy(UpDataA.Updatabuff + ((UpDataA.XmodemNum - 1) % (STM32_PAGE_SIZE / 128)) * 128,
					   &data[3], 128);
				if((UpDataA.XmodemNum % (STM32_PAGE_SIZE / 128)) == 0)
				{
					if(BootStaFlag&CMD5_XMODEM_FLAG)
					{
						for(i=0;i<4;i++)
						{
							W25Q64_PageWrite(&UpDataA.Updatabuff[i*256], 
							(UpDataA.XmodemNum/8 - 1) * 4 + i + UpDataA.W25Q64_BlockNB * 64 *4);
						}
					}
					else STM32_WriteFlash(STM32_A_SADDR + ((UpDataA.XmodemNum / (STM32_PAGE_SIZE / 128)) - 1) * STM32_PAGE_SIZE,
									 (uint32_t *)UpDataA.Updatabuff,
									 STM32_PAGE_SIZE);              /* 写入到单片机A区对应位置 */
				}
				u1_printf("\x06");                              /* ACK：包通过，请发下一包 */
			}
			else
			{
				u1_printf("\x15");                              /* NAK：CRC错了，重发这包 */
			}
		}
		if((datalen == 1) && (data[0] == 0x04))                     /* EOT：全部传输结束 */
		{
			u1_printf("\x06");
			if((UpDataA.XmodemNum % (STM32_PAGE_SIZE / 128)) != 0)
			{
				if(BootStaFlag&CMD5_XMODEM_FLAG)
				{
					for(i=0;i<4;i++)
					{
						W25Q64_PageWrite(&UpDataA.Updatabuff[i*256], 
						(UpDataA.XmodemNum/8) * 4 + i + UpDataA.W25Q64_BlockNB * 64 *4);
					}
				}
				else STM32_WriteFlash(STM32_A_SADDR + ((UpDataA.XmodemNum / (STM32_PAGE_SIZE / 128))) * STM32_PAGE_SIZE,
								 (uint32_t *)UpDataA.Updatabuff,
								 (UpDataA.XmodemNum % (STM32_PAGE_SIZE / 128)) * 128);
			}
			BootStaFlag &= ~IAP_XMODEMD_FLAG;
			if(BootStaFlag&CMD5_XMODEM_FLAG)
			{
				BootStaFlag &= ~CMD5_XMODEM_FLAG;
				OTA_Info.Firelen[UpDataA.W25Q64_BlockNB] = UpDataA.XmodemNum * 128;
				W25Q64_WriteOTAInfo();
				Delay_ms(100);
				BootLoader_Info();
			}
			else
			{
				Delay_ms(100);
				NVIC_SystemReset();  
			}
		}
	}
	else if(BootStaFlag & SET_VERSION_FLAG)
	{
		if(datalen == 26)
		{
			if(sscanf((char *)data, "VER=%d.%d.%d-%d/%d/%d-%d:%d",
					  &temp, &temp, &temp, &temp, &temp, &temp, &temp, &temp) == 8)
			{
				memset(OTA_Info.OTA_ver, 0, 32);
				memcpy(&OTA_Info.OTA_ver, data, 26);
				W25Q64_WriteOTAInfo();
				u1_printf("版本正确\r\n");
				BootStaFlag &= ~SET_VERSION_FLAG;
				BootLoader_Info();
			}
			else u1_printf("版本号格式错误\r\n");
		}
		else
		{
			u1_printf("版本号长度错误\r\n");
		}
	}
	else if(BootStaFlag & CMD_5_FLAG)
	{
		if(datalen == 1)
		{
			if((data[0]>=0x31)&&(data[0]<=0x39))
			{
				UpDataA.W25Q64_BlockNB = data[0] - 0x30;    /* ASCII数字转块号，如发'5'就选5号块 */
				BootStaFlag |= (IAP_XMODEMC_FLAG | IAP_XMODEMD_FLAG | CMD5_XMODEM_FLAG);
				UpDataA.XmodemTimer = 0;                    /* 'C'心跳计时清零 */
				UpDataA.XmodemNum = 0;                       /* 包计数清零 */
				OTA_Info.Firelen[UpDataA.W25Q64_BlockNB] = 0;
				u1_printf("正在擦除外部Flash第%d块...\r\n", UpDataA.W25Q64_BlockNB);
				for(i = 0; i < 16; i++)    /* 64KB = 16个4KB扇区，全部擦成FF */
				{
					W25Q64_Erase4K(UpDataA.W25Q64_BlockNB * 64 * 1024 + i * W25Q64_SECTOR_SIZE);
				}
				u1_printf("通过Xmodem协议，向外部Flash第%d个块下载程序，请使用bin格式文件\r\n",
						  UpDataA.W25Q64_BlockNB);          /* u0_printf → u1_printf */
				BootStaFlag &= ~CMD_5_FLAG;                 /* 防止重复进入，处理完就清 */
			}
			else u1_printf("编号错误\r\n");
		}
		else
		{
			u1_printf("数据长度错误\r\n");
		}
	}
	else if(BootStaFlag & CMD_6_FLAG)
	{
		if(datalen == 1)
		{
			if((data[0]>=0x31)&&(data[0]<=0x39))
			{
				UpDataA.W25Q64_BlockNB = data[0] - 0x30; 
				BootStaFlag |= UPDATA_A_FLAG;
				BootStaFlag &= ~CMD_6_FLAG; 		
			}
			else u1_printf("编号错误\r\n");
		}
		else
		{
			u1_printf("数据长度错误\r\n");
		}
	}

}

/**
BootLoader命令行菜单
 */
void BootLoader_Info(void)
{
    u1_printf("\r\n");
    u1_printf("[1]擦除A区\r\n");
    u1_printf("[2]串口IAP下载A区程序\r\n");
    u1_printf("[3]设置OTA版本号\r\n");
    u1_printf("[4]查询OTA版本号\r\n");
    u1_printf("[5]向外部Flash下载程序\r\n");
    u1_printf("[6]使用外部Flash内程序\r\n");
    u1_printf("[7]重启\r\n");
}

void W25Q64_ReadOTAInfo(void)
{
    memset(&OTA_Info, 0, OTA_INFOCB_SIZE);
    W25Q64_Read((uint8_t *)&OTA_Info, W25Q64_META_SADDR, OTA_INFOCB_SIZE);
}

/**
把 OTA_Info元数据写入W25Q64最后一个扇区
 */
void W25Q64_WriteOTAInfo(void)
{
    uint8_t pagebuf[256];                   
    uint8_t ret;

    ret = W25Q64_Erase4K(W25Q64_META_SADDR); 
    if(ret != W25Q64_OK)
    {
        u1_printf("[WriteOTAInfo] 扇区擦除失败 ret=%d\r\n", ret);
        return;
    }

    memset(pagebuf, 0xFF, sizeof(pagebuf));
    memcpy(pagebuf, &OTA_Info, OTA_INFOCB_SIZE);  

    ret = W25Q64_PageWrite(pagebuf, W25Q64_META_PAGE);
    if(ret != W25Q64_OK)
        u1_printf("[WriteOTAInfo] 页编程失败 ret=%d\r\n", ret);
}

/* 设置主堆栈指针：内联汇编，Keil AC5 语法 */
__asm void MSR_SP(uint32_t addr)
{
    MSR MSP, R0    
    BX  R14        
}

void BootLoader_Clear(void)
{
    USART_DeInit(USART1); 
    GPIO_DeInit(GPIOA);    
    GPIO_DeInit(GPIOB);
}

/**
跳转到 App 区
 */
void LOAD_A(uint32_t addr)
{
    //合法性检查App镜像的第一个字是栈顶指针，必须落在 RAM 区
    if((*(uint32_t *)addr) >= 0x20000000 && (*(uint32_t *)addr) <= 0x20004FFF)
    {
        MSR_SP(*(uint32_t *)addr);            
        load_A = (load_a)(*(uint32_t *)(addr + 4)); 
        BootLoader_Clear();                    
        load_A(0);                               
    }
	else
	{
		u1_printf("跳转A区失败\r\n");
		u1_printf("A区可能已被擦除！！\r\n");	
	}
}
/**
XmodemCRC16校验
 */
uint16_t Xmodem_CRC16(uint8_t *data, uint16_t datalen)
{
    uint8_t i;
    uint16_t Crcinit = 0x0000;
    uint16_t Crcipoly = 0x1021;

    while(datalen--)
    {
        Crcinit = (*data << 8) ^ Crcinit;
        for(i=0;i<8;i++)
        {
            if(Crcinit&0x8000)
                Crcinit = (Crcinit << 1) ^ Crcipoly;
            else
                Crcinit = (Crcinit << 1);
        }
        data++;
    }
    return Crcinit;
}





    
