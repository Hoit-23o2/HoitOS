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
** ��        ��: RAMģ��NorFlash����
*********************************************************************************************************/

#ifndef SYLIXOS_DRIVER_MTD_NOR_FAKE_NOR_H_
#define SYLIXOS_DRIVER_MTD_NOR_FAKE_NOR_H_


#include <stdio.h>
#include <stdlib.h>
#include "SST39VF1601.h"


/*********************************************************************************************************
  ��¼ÿ��Sector�Ĳ�д����
*********************************************************************************************************/
typedef struct sector_info
{
    INT32 erase_cnt;
    BOOL  is_bad;
} sector_info_t;

sector_info_t* sector_infos;
/*********************************************************************************************************
  ģ��nor_flash
*********************************************************************************************************/
PCHAR nor_flash_base_addr;
#define GET_NOR_FLASH_ADDR(offset)            (nor_flash_base_addr + ((UINT)offset))                        /* ����Ƭ��ƫ�ƻ�ȡ��ʵ��ַ */
#define GET_NOR_FLASH_BLK_ADDR(block_no)      GET_NOR_FLASH_ADDR(((block_no) * NOR_FLASH_BLKSZ))                
#define GET_NOR_FLASH_SECTOR_ADDR(sector_no)  GET_NOR_FLASH_ADDR(((sector_no) * NOR_FLASH_SECTORSZ))     
#define GET_NOR_FLASH_SECTOR(offset)          (((UINT)offset) / NOR_FLASH_SECTORSZ)                         /* ����Ƭ�ڵ�ַ��ȡSECTOR�ţ�0 ~ 511�� */
#define GET_NOR_FLASH_BLK(offset)             (GET_NOR_FLASH_SECTOR((offset)) / NOR_FLASH_SECTPBLK)         /* ����Ƭ�ڵ�ַ��ȡBLOCK�ţ�0 ~ 31�� */

/*********************************************************************************************************
  ����ѡ��
*********************************************************************************************************/
typedef enum erase_options
{
    ERASE_CHIP,
    ERASE_BLOCK,
    ERASE_SECTOR
} ERASE_OPTIONS;

/*********************************************************************************************************
  ��ȡѡ��
*********************************************************************************************************/
typedef enum read_options
{
    READ_SECTOR,
    READ_BYTE
} READ_OPTIONS;

typedef struct read_content
{
    PCHAR content;
    UINT  size;
    BOOL  is_success;
} read_content_t;

/*********************************************************************************************************
  SST39VF1601 ��������
*********************************************************************************************************/
VOID    nor_init(VOID);
VOID    scan_nor(VOID);
VOID    erase_nor(UINT offset, ERASE_OPTIONS ops);
VOID    write_nor(UINT offset, PCHAR content, UINT size);
read_content_t   read_nor(UINT offset, READ_OPTIONS ops);
VOID    reset_nor(VOID);

#endif /* SYLIXOS_DRIVER_MTD_NOR_FAKE_NOR_H_ */
