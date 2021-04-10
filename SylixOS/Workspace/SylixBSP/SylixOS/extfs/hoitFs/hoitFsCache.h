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
** 文   件   名: hoitCache.h
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 04 月 02 日
**
** 描        述: 缓存层
*********************************************************************************************************/

#ifndef SYLIXOS_EXTFS_HOITFS_HOITFSCACHE_H_
#define SYLIXOS_EXTFS_HOITFS_HOITFSCACHE_H_

#include "hoitType.h"

BOOL     hoitEnableCache(UINT8 uiCacheBlockSize, UINT8 uiCacheBlockNums);
BOOL     hoitReadFromCache(UINT32 uiOfs, PCHAR pContent, UINT32 uiSize);
BOOL     hoitWriteToCache(UINT32 uiOfs, PCHAR pContent, UINT32 uiSize);
BOOL     hoitFlushCache();

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSCACHE_H_ */
