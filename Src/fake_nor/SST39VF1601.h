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
** �ļ���������: 2021 �� 02 �� 24 ��
**
** ��        ��: RAMģ��NorFlash���� SST39VF1601 ���
*********************************************************************************************************/


#ifndef SYLIXOS_DRIVER_MTD_NOR_SST39VF1601_H_
#define SYLIXOS_DRIVER_MTD_NOR_SST39VF1601_H_

/*********************************************************************************************************
  SST39VF1601 ���ù��
*********************************************************************************************************/
#define NOR_FLASH_NM                    "SST39VF1601 Power NOR"
#define NOR_FLASH_SZ                    (2 * 1024 * 1024)                  /*  Nor Flash Size 2MB */
#define NOR_FLASH_NBLK                  32
#define NOR_FLASH_BLKSZ                 (NOR_FLASH_SZ / NOR_FLASH_NBLK)
#define NOR_FLASH_SECTPBLK              16
#define NOR_FLASH_NSECTOR               (NOR_FLASH_NBLK * NOR_FLASH_SECTPBLK)               
#define NOR_FLASH_SECTORSZ              (4 * 1024)                         /* 4KB per sector */
#define NOR_FLASH_MAX_ERASE_CNT         1                                  /* ���������� */

#endif /* SYLIXOS_DRIVER_MTD_NOR_SST39VF1601_H_ */
