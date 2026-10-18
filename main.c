#include "stm32f10x.h"                  // Device header
#include "Serial.h"
#include "delay.h"
#include "spi.h"
#include "w25.h"
#include "flash.h"
#include "main.h"
#include "boot.h"

OTA_InfoCB OTA_Info;
UpDataA_CB UpDataA;
uint32_t BootStaFlag = 0;           

int main(void)
{
    uint8_t i;

    Serial_Init(115200);               
    W25Q64_Init();                      

    W25Q64_ReadOTAInfo();   
	
    BootLoader_Branch();

    while(1)
    {
		Delay_ms(10);
		if(U1io.data_out != U1io.data_in)
		{
			BootLoader_Event(U1io.data_out->start,
							 U1io.data_out->end - U1io.data_out->start + 1);
			U1io.data_out++;
			if(U1io.data_out == U1io.data_end)
			{
				U1io.data_out = &U1io.data[0];
			}
		}
		if(BootStaFlag & IAP_XMODEMC_FLAG){        // 已通过菜单[2]进入Xmodem接收模式
			if(UpDataA.XmodemTimer >= 100)
			{      
				u1_printf("C");                    // 发邀请字符
				UpDataA.XmodemTimer = 0;
			}
			UpDataA.XmodemTimer++;                // 每圈+1
		}

		
        if(BootStaFlag & UPDATA_A_FLAG)
        {
					u1_printf("长度%d字节\r\n", OTA_Info.Firelen[UpDataA.W25Q64_BlockNB]);  //更新A区

            if(OTA_Info.Firelen[UpDataA.W25Q64_BlockNB] % 4 == 0)   //长度4对齐
            {

                STM32_EraseFlash(STM32_A_START_PAGE, STM32_A_PAGE_NUM);   //先擦除

        
                for(i = 0; i < OTA_Info.Firelen[UpDataA.W25Q64_BlockNB] / STM32_PAGE_SIZE; i++)
                {
                    W25Q64_Read(UpDataA.Updatabuff,
                                i*1024 + UpDataA.W25Q64_BlockNB*64*1024,
                                STM32_PAGE_SIZE);
                    STM32_WriteFlash(STM32_A_SADDR + i*STM32_PAGE_SIZE,
                                     (uint32_t *)UpDataA.Updatabuff,
                                     STM32_PAGE_SIZE);
                }

         
                if(OTA_Info.Firelen[UpDataA.W25Q64_BlockNB] % 1024 != 0)   //不足一整页，把余下的单独写入
                {
                    W25Q64_Read(UpDataA.Updatabuff,
                                i*1024 + UpDataA.W25Q64_BlockNB*64*1024,
                                OTA_Info.Firelen[UpDataA.W25Q64_BlockNB] % 1024);
                    STM32_WriteFlash(STM32_A_SADDR + i*STM32_PAGE_SIZE,
                                     (uint32_t *)UpDataA.Updatabuff,
                                     OTA_Info.Firelen[UpDataA.W25Q64_BlockNB] % 1024);
                }

                if(UpDataA.W25Q64_BlockNB == 0)   //清除OTA标志
                {
                    OTA_Info.OTA_flag = 0;
                    W25Q64_WriteOTAInfo();   
                }
                u1_printf("更新完成\r\n");
                NVIC_SystemReset();        //复位重启
            }
            else
            {
                u1_printf("长度错误\r\n");
							BootStaFlag &= ~UPDATA_A_FLAG;  //置0位，长度不合规，停在原地
            }
        }
    }
}
