/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: nor.c
**
** 创   建   人: Pan yanqi (潘延麒)
**
** 文件创建日期: 2021 年 03 月 11 日
**
** 描        述: NorFlash裸板驱动
*********************************************************************************************************/
#ifndef SYLIXOS_DRIVER_MTD_NOR_NOR_H_
#define SYLIXOS_DRIVER_MTD_NOR_NOR_H_

#include "Am29LV160DB.h"
#include "nor_util.h"


/*********************************************************************************************************
  Mini2440 NorFlash基地址
  这里要为其计算，因为SylixOS只能写入虚拟地址，需要用API_VmmIoRemap2将其映射到某个地方
*********************************************************************************************************/
UINT32 NOR_FLASH_BASE;
/*********************************************************************************************************
  NorFlash启动模式
*********************************************************************************************************/
typedef enum nor_init_flag
{
    INIT_FAKE_NOR,
    INIT_TRUE_NOR
} ENUM_NOR_INIT_FLAG;

ENUM_NOR_INIT_FLAG _G_nor_flash_init_flag;

#define FAKE_MODE()             _G_nor_flash_init_flag = INIT_FAKE_NOR
#define TRUE_MODE()             _G_nor_flash_init_flag = INIT_TRUE_NOR
#define IS_FAKE_MODE()          _G_nor_flash_init_flag == INIT_FAKE_NOR
/*********************************************************************************************************
  Mini2440 NorFLash操作 
*********************************************************************************************************/
VOID             nor_init(ENUM_NOR_INIT_FLAG nor_init_flag);
VOID             scan_nor();
UINT8            erase_nor(UINT offset, ENUM_ERASE_OPTIONS ops);
UINT8            write_nor(UINT offset, PCHAR content, UINT size_bytes, ENUM_WRITE_OPTIONS ops);
UINT8            read_nor(UINT offset, CHAR* content, UINT size_bytes);

#ifdef NOR_TEST
/*********************************************************************************************************
  测试函数
*********************************************************************************************************/
BOOL             test_nor();
#endif // NOR_TEST

#endif /* SYLIXOS_DRIVER_MTD_NOR_NOR_H_ */
