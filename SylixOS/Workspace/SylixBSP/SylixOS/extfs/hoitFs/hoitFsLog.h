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

#define LOG_DEBUG

#define S_IFLOG     0xd000



typedef struct hoitLog
{
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;
    mode_t              file_type;
    UINT                ino;
    UINT                version;

    UINT                uiLogSize;
    UINT                uiLogAddr;
} HOIT_RAW_LOG;

typedef HOIT_RAW_LOG * PHOIT_RAW_LOG;

/*********************************************************************************************************
  LOG�ļ���غ���
*********************************************************************************************************/
PHOIT_LOG_INFO              hoitLogInit(PHOIT_VOLUME pfs, UINT uiLogSize, UINT uiSectorNum);
PHOIT_LOG_INFO              hoitLogOpen(PHOIT_VOLUME pfs, PHOIT_RAW_LOG pRawLog);
VOID                        hoitLogRead(PHOIT_VOLUME pfs, PHOIT_LOG_INFO pLogInfo, UINT uiOfs, PCHAR pLog, UINT uiSize);
VOID                        hoitLogAppend(PHOIT_VOLUME pfs, PCHAR pLog);


#endif /* SYLIXOS_EXTFS_HOITFS_HOITLOG_H_ */
