#ifndef __MAIN_H
#define __MAIN_H


/* ===================== STM32 内部 Flash 分区（OTA 用） ===================== */
#define STM32_FLASH_SADDR      0x08000000                          //内部Flash首地址
#define STM32_PAGE_SIZE        1024                               
#define STM32_PAGE_NUM         64                                  
#define STM32_B_PAGE_NUM       20                                  
#define STM32_A_PAGE_NUM       (STM32_PAGE_NUM - STM32_B_PAGE_NUM) //App区页数44页
#define STM32_A_START_PAGE     (STM32_B_PAGE_NUM)                    //App 起始页号20
#define STM32_A_SADDR          (STM32_FLASH_SADDR + STM32_A_START_PAGE * STM32_PAGE_SIZE)  //App起始地址0x08005000

/* ===================== W25Q64 存储布局 ===================== */
#define W25Q64_TOTAL_SIZE      0x800000                            //总容量 8MB
#define W25Q64_SECTOR_SIZE     4096                                
#define W25Q64_PAGE_SIZE       256                                 //编程页 256B（不可跨页写）
#define W25Q64_SECTOR_NUM      (W25Q64_TOTAL_SIZE / W25Q64_SECTOR_SIZE)  
#define W25Q64_PAGE_NUM        (W25Q64_TOTAL_SIZE / W25Q64_PAGE_SIZE)    

/* ---------- 元数据区：最后一个扇区（代替 AT24C02） ---------- */
#define W25Q64_META_SADDR      (W25Q64_TOTAL_SIZE - W25Q64_SECTOR_SIZE)   // 元数据扇区首地址0x7FF000
#define W25Q64_META_PAGE       (W25Q64_META_SADDR / W25Q64_PAGE_SIZE)    
#define W25Q64_META_PADDR      (W25Q64_META_SADDR + 0)                    // 元数据页首地址 = 0x7FF000 */

/* ---------- 固件下载区（OTA 备用，从头开始） ---------- */
#define W25Q64_FW_SADDR        0x000000                            //固件在W25Q64存放首地址
#define OTA_SET_FLAG      	0xAABB1122     //OTA更新标志魔数
#define UPDATA_A_FLAG     	0x00000001     //Bootloader状态位：正在更新A区（搬运固件中）
#define IAP_XMODEMC_FLAG	0x00000002
#define IAP_XMODEMD_FLAG	0x00000004
#define SET_VERSION_FLAG	0x00000008
#define CMD_5_FLAG			0x00000010
#define CMD5_XMODEM_FLAG	0x00000020
#define CMD_6_FLAG			0x00000040


typedef struct{
    uint32_t OTA_flag;                   
    uint32_t Firelen[11];                
	uint8_t OTA_ver[32];
}OTA_InfoCB;                           

#define OTA_INFOCB_SIZE    sizeof(OTA_InfoCB)

typedef struct{
    uint8_t  Updatabuff[STM32_PAGE_SIZE]; //一页1024，写满
    uint32_t W25Q64_BlockNB;              //数据写入W25Q64的页号
	uint32_t XmodemTimer;
	uint32_t XmodemNum;
	uint32_t XmodemCRC;
}UpDataA_CB;

extern OTA_InfoCB OTA_Info;
extern UpDataA_CB UpDataA;
extern uint32_t  BootStaFlag;   

#endif
