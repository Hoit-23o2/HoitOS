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

#define S_IFLOG     0xd000



/*********************************************************************************************************
  LOG�ļ���غ���
*********************************************************************************************************/
PHOIT_LOG_INFO              hoitLogInit(PHOIT_VOLUME pfs, UINT uiLogSize, UINT uiSectorNum);
PHOIT_LOG_INFO              hoitLogOpen(PHOIT_VOLUME pfs, PHOIT_RAW_LOG pRawLog);
PCHAR                       hoitLogEntityGet(PHOIT_VOLUME pfs, UINT uiEntityNum);
VOID                        hoitLogAppend(PHOIT_VOLUME pfs, PCHAR pcEntityContent, UINT uiEntitySize);

#ifdef LOG_TEST
VOID                        hoitLogTest();
#endif // LOG_TEST
#endif /* SYLIXOS_EXTFS_HOITFS_HOITLOG_H_ */
