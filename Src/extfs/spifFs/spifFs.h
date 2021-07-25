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
** ��   ��   ��: spifFs.h
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 06 �� 01��
**
** ��        ��: Spiffs�ļ�ϵͳ�ӿڲ�
*********************************************************************************************************/

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL

#ifndef SYLIXOS_EXTFS_SPIFFS_SPIFFS_H_
#define SYLIXOS_EXTFS_SPIFFS_SPIFFS_H_

#include "SylixOS.h"
#include "spifFsType.h"
#include "spifFsGlue.h"
#include "../../driver/mtd/nor/nor.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0// && LW_CFG_RAMFS_EN > 0
/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API INT          API_SpifFsDrvInstall(VOID);
LW_API INT          API_SpifFsDevCreate(PCHAR   pcName, PLW_BLK_DEV  pblkd);
LW_API INT          API_SpifFsDevDelete(PCHAR   pcName);

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0 */
#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFFS_H_ */
