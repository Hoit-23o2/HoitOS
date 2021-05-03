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
** ��   ��   ��: hoitFsGC.h
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 04 �� 25 ��
**
** ��        ��: ��־��ʵ��
*********************************************************************************************************/

#ifndef SYLIXOS_EXTFS_HOITFS_HOITLOG_H_
#define SYLIXOS_EXTFS_HOITFS_HOITLOG_H_

#include "hoitType.h"

VOID    hoitLogInit(HOIT_VOLUME pfs, UINT uiLogSize);
VOID    hoitLogOpen(HOIT_VOLUME pfs);
VOID    hoitLogRead(HOIT_VOLUME pfs, UINT uiOfs, PCHAR pLog, UINT uiSize);
VOID    hoitLogAppend(HOIT_VOLUME pfs, PCHAR pLog);


#endif /* SYLIXOS_EXTFS_HOITFS_HOITLOG_H_ */
