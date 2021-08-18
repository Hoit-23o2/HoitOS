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
** ��   ��   ��: fake_nor_cmd.c
**
** ��   ��   ��: Pan yanqi (������)
**
** �ļ���������: 2021 �� 02 �� 25 ��
**
** ��        ��: RAMģ��NorFlash���� ���� ���ڵ���BUG
*********************************************************************************************************/

#ifndef SYLIXOS_DRIVER_MTD_NOR_FAKE_NOR_CMD_H_
#define SYLIXOS_DRIVER_MTD_NOR_FAKE_NOR_CMD_H_

#include <stdio.h>
#include "SST39VF1601.h"

#define SHOW_DIVIDER()        printf("--------------------------------------------------------------------------------\n")
#define GET_PREFIX()          *(ppcArgV)
#define GET_ARG(i)            *(ppcArgV + i)
#define LOWER_STR(p)          while(*p != '\0'){              \
                                if(*p >= 'A' && *p <= 'Z')    \
                                      *p += 32;               \
                                  p++;                        \
                              }                               \
/*********************************************************************************************************
  ΪNorFlashע������
*********************************************************************************************************/
VOID register_nor_cmd();
VOID pretty_print(PCHAR header, PCHAR content);
/*********************************************************************************************************
  ֧������
*********************************************************************************************************/
typedef enum cmd_type
{
  SUMMARY,                                              /* �鿴Sector�Ĳ�д��� */
  WRITE,                                                /* д */
  ERASE,                                                /* ���� */
  READ,                                                 /* �� */
  HELP,                                                 /* �����ֲ� */
  TEST,                                                 /* ������Ϊ */
  RESET                                                 /* ����nor flash */
} CMD_TYPE;
#endif /* SYLIXOS_DRIVER_MTD_NOR_FAKE_NOR_CMD_H_ */