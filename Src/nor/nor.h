/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: nor.c
**
** ��   ��   ��: Pan yanqi (������)
**
** �ļ���������: 2021 �� 03 �� 11 ��
**
** ��        ��: NorFlash�������
*********************************************************************************************************/
#ifndef SYLIXOS_DRIVER_MTD_NOR_NOR_H_
#define SYLIXOS_DRIVER_MTD_NOR_NOR_H_

#include "Am29LV160DB.h"
#include "nor_util.h"


/*********************************************************************************************************
  Mini2440 NorFlash����ַ
  ����ҪΪ����㣬��ΪSylixOSֻ��д�������ַ����Ҫ��API_VmmIoRemap2����ӳ�䵽ĳ���ط�
*********************************************************************************************************/
UINT32 NOR_FLASH_BASE;
/*********************************************************************************************************
  NorFlash����ģʽ
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
  Mini2440 NorFLash���� 
*********************************************************************************************************/
VOID             nor_init(ENUM_NOR_INIT_FLAG nor_init_flag);
VOID             scan_nor();
UINT8            erase_nor(UINT offset, ENUM_ERASE_OPTIONS ops);
UINT8            write_nor(UINT offset, PCHAR content, UINT size_bytes, ENUM_WRITE_OPTIONS ops);
UINT8            read_nor(UINT offset, CHAR* content, UINT size_bytes);

#ifdef NOR_TEST
/*********************************************************************************************************
  ���Ժ���
*********************************************************************************************************/
BOOL             test_nor();
#endif // NOR_TEST

#endif /* SYLIXOS_DRIVER_MTD_NOR_NOR_H_ */