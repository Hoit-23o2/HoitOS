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
** 描        述: 垃圾回收实现
*********************************************************************************************************/
#ifndef SYLIXOS_EXTFS_HOITFS_HOITGC_H_
#define SYLIXOS_EXTFS_HOITFS_HOITGC_H_

#include "hoitType.h"

#define GC_DEBUG

//TODO:注意 当删除RawInfo的时候，一定要记得调整它属于的GC_Sector的属性内容

VOID    hoitFsGCBackgroundThread(PHOIT_VOLUME pfs);
VOID    hoitFsGCForgroudForce(PHOIT_VOLUME pfs);
VOID    hoitFsGCThread(PHOIT_VOLUME pfs);

#endif /* SYLIXOS_EXTFS_HOITFS_HOITGC_H_ */
