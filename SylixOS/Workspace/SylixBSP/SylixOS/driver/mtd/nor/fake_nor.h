/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: fake_nor.c
**
** ��   ��   ��: Pan yanqi (������)
**
** �ļ���������: 2021 �� 03 �� 17 ��
**
** ��        ��: RAMģ��NorFlash������������ݶ���
*********************************************************************************************************/

#ifndef SYLIXOS_DRIVER_MTD_NOR_FAKE_NOR_H_
#define SYLIXOS_DRIVER_MTD_NOR_FAKE_NOR_H_

#include "Am29LV160DB.h"
#include <stdio.h>
#include <stdlib.h>


<<<<<<< HEAD
#define NOR_FLASH_MAX_ERASE_CNT                 10000
=======
#define NOR_FLASH_MAX_ERASE_CNT                 1000
>>>>>>> 3d721479faf46c7ab9d923e5b31785af351d8932
/*********************************************************************************************************
  ��¼ÿ��Sector�Ĳ�д����
*********************************************************************************************************/
typedef struct sector_info
{
    INT32 erase_cnt;
    BOOL  is_bad;
} sector_info_t;

sector_info_t* sector_infos;


VOID nor_summary();
VOID generate_bad_sector();
VOID assign_sector_bad(INT sector_no);
BOOL get_sector_is_bad(INT sector_no);
#endif /* SYLIXOS_DRIVER_MTD_NOR_FAKE_NOR_H_ */
