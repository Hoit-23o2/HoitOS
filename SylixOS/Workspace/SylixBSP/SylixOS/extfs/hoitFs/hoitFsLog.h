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

VOID    hoitLogInit(HOIT_VOLUME pfs, UINT uiLogSize);
VOID    hoitLogOpen(HOIT_VOLUME pfs);
VOID    hoitLogRead(HOIT_VOLUME pfs, UINT uiOfs, PCHAR pLog, UINT uiSize);
VOID    hoitLogAppend(HOIT_VOLUME pfs, PCHAR pLog);


#endif /* SYLIXOS_EXTFS_HOITFS_HOITLOG_H_ */
