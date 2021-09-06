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
** ��   ��   ��: nor_util.c
**
** ��   ��   ��: Pan yanqi (������)
**
** �ļ���������: 2021 �� 03 �� 11 ��
**
** ��        ��: NorFlash����������߿�
*********************************************************************************************************/

#ifndef SYLIXOS_DRIVER_MTD_NOR_NOR_UTIL_H_
#define SYLIXOS_DRIVER_MTD_NOR_NOR_UTIL_H_

#include <stdio.h>
#include "Am29LV160DB.h"

#define NMAX_DISPLAY                            80
#define TEMP_BUF_SZ                             80
#define NOR_FLASH_SECTOR_INFO_FILE_PATH         "/tmp/nor_sector_infos"
/*********************************************************************************************************
  �����ʾ����
*********************************************************************************************************/
#define FAIL "FAIL: "
#define WARN "WARN: "
#define INFO "INFO: "
/*********************************************************************************************************
  �����ڴ��д����
*********************************************************************************************************/
VOID     write_word_to_mem(UINT32 base, UINT32 offset, UINT16 data);
UINT16   read_word_from_mem(UINT32 base, UINT32 offset);
UINT8    read_byte_from_mem(UINT32 base, UINT32 offset);
VOID     wait_ready(UINT32 base, UINT32 addr);
/*********************************************************************************************************
  ��װ�����
*********************************************************************************************************/
VOID     nor_command_unlock(UINT32 base);
VOID     nor_reset(UINT32 base);
VOID     nor_write_buffer(UINT32 base, UINT offset, PCHAR _content, UINT size_bytes);
VOID     nor_erase_sector(UINT32 base, UINT offset);
UINT8    nor_erase_range(UINT8 low_sector_no, UINT8 high_sector_no, UINT8 (*erase_nor)(UINT, ENUM_ERASE_OPTIONS));
UINT8    nor_erase_region(INT8 region_no, UINT8 (*erase_nor)(UINT, ENUM_ERASE_OPTIONS));
BOOL     nor_check_offset_range(UINT32 base, UINT32 offset, UINT size_bytes);
BOOL     nor_check_modifiable_perm(UINT32 offset);
BOOL     nor_check_should_erase(UINT32 base, UINT offset, PCHAR content, UINT size_bytes);
/*********************************************************************************************************
  ������ʾ����
*********************************************************************************************************/
#define  DONT_CENTRAL            FALSE
#define  DO_CENTRAL              TRUE
VOID     pretty_print(PCHAR header, PCHAR content, BOOL centralized);

VOID     show_divider(PCHAR header);

#endif /* SYLIXOS_DRIVER_MTD_NOR_NOR_UTIL_H_ */
