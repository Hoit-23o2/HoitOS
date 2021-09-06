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

// #define NOR_DEBUG
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
  ���õĺ����������
*********************************************************************************************************/
static inline UINT8 GET_SECTOR_NO(UINT32 offset){
    UINT i;
    for (i = 0; i < NOR_FLASH_NSECTOR; i++)                             
    {                                                                       
        if(_G_am29LV160DB_sector_infos[i].sector_start_offset <= offset &&  
            _G_am29LV160DB_sector_infos[i].sector_end_offset >= offset){    
            return i;                                                         
        }                                                                   
    }
    return -1;
}

static inline UINT32 GET_SECTOR_OFFSET(UINT8 sector_no){
    if(sector_no < 0 || sector_no >= NOR_FLASH_NSECTOR){
        return -1;
    }
    return _G_am29LV160DB_sector_infos[sector_no].sector_start_offset;
}

static inline UINT GET_SECTOR_SIZE(UINT8 sector_no){
    if(sector_no < 0 || sector_no >= NOR_FLASH_NSECTOR){
        return -1;
    }
    return 1024 * (_G_am29LV160DB_sector_infos[sector_no].sector_size);     
}

static inline UINT8 GET_SECTOR_REGION(UINT8 sector_no){
    if(sector_no < 0 || sector_no >= NOR_FLASH_NSECTOR){
        return -1;
    }
    return _G_am29LV160DB_sector_infos[sector_no].region_no;     
}


static inline BOOL IS_SECTOR_DIRTY(UINT32 base, UINT8 sector_no){
    INT i;
    if(sector_no < 0 || sector_no >= NOR_FLASH_NSECTOR){
        return TRUE;
    }
    UINT sector_size = GET_SECTOR_SIZE(sector_no);
    UINT32 addr = GET_SECTOR_OFFSET(sector_no) + base;
    volatile UINT8* p;
    for (i = 0; i < sector_size; i++)
    {
        p = (volatile UINT8*)(addr + i);
        if((*p & 0xFF) != 0xFF){
          return TRUE;
        }
    }
    return FALSE;
}

#define NOR_FLASH_START_OFFSET                     (GET_SECTOR_OFFSET(GET_SECTOR_NO(UBOOT_SIZE) + 1))
/*********************************************************************************************************
  Mini2440 NorFLash���� 
*********************************************************************************************************/
VOID             nor_init(ENUM_NOR_INIT_FLAG nor_init_flag);
VOID             scan_nor();
UINT8            erase_nor(UINT offset, ENUM_ERASE_OPTIONS ops);
UINT8            write_nor(UINT offset, PCHAR content, UINT size_bytes, ENUM_WRITE_OPTIONS ops);
UINT8            read_nor(UINT offset, PCHAR content, UINT size_bytes);

#ifdef NOR_TEST
/*********************************************************************************************************
  ���Ժ���
*********************************************************************************************************/
BOOL             test_nor();
#endif // NOR_TEST

#endif /* SYLIXOS_DRIVER_MTD_NOR_NOR_H_ */
