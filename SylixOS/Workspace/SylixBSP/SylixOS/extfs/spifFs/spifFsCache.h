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
** 文   件   名: spifFsCache.h
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 01日
**
** 描        述: Spiffs文件系统缓存层
*********************************************************************************************************/
#ifndef SYLIXOS_EXTFS_SPIFFS_SPIFFSCACHE_H_
#define SYLIXOS_EXTFS_SPIFFS_SPIFFSCACHE_H_

#include "spifFsType.h"

INT32 spiffsCacheFflush(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler);
PSPIFFS_CACHE_PAGE spiffsCachePageGetByFd(PSPIFFS_VOLUME pfs, PSPIFFS_FD pFd);
#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFSCACHE_H_ */
