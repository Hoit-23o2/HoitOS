/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: hoitFsGC.h
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 04 月 25 日
**
** 描        述: 日志层实现
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
    UINT                uiLogFirstAddr;
} HOIT_RAW_LOG;

typedef HOIT_RAW_LOG * PHOIT_RAW_LOG;

/*********************************************************************************************************
  LOG文件相关函数
*********************************************************************************************************/
PHOIT_LOG_INFO              hoitLogInit(PHOIT_VOLUME pfs, UINT uiLogSize, UINT uiSectorNum);
PHOIT_LOG_INFO              hoitLogOpen(PHOIT_VOLUME pfs, PHOIT_RAW_LOG pRawLog);
UINT                        hoitLogEntityGet(PHOIT_VOLUME pfs, UINT uiEntityNum);
VOID                        hoitLogAppend(PHOIT_VOLUME pfs, PCHAR pLog);
BOOL                        hoitLogCheckIfLog(PHOIT_VOLUME pfs, PHOIT_ERASABLE_SECTOR pErasableSector);

#endif /* SYLIXOS_EXTFS_HOITFS_HOITLOG_H_ */
