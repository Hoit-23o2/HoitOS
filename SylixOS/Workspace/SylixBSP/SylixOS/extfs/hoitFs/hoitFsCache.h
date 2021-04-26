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
#include "SylixOS.h"
/*********************************************************************************************************
 * 结构体
*********************************************************************************************************/

typedef struct HOIT_CACHE_HDR
{
    UINT8               HOITCACHE_blockSize;    /* 单个cache大小 */
    UINT8               HOITCACHE_blockNums;    /* cache最大数量 */
    LW_OBJECT_HANDLE    HOITCACHE_hVolLock;     /* cache自旋锁 */
}HOIT_CACHE_HDR;
typedef HOIT_CACHE_HDR * PHOIT_CACHE_HDR;

typedef struct HOIT_CACHE_BLK
{
    BOOL        HOITBLK_bValid;
    PCHAR       HOITBLK_buf;
}HOIT_CACHE_BLK;
typedef HOIT_CACHE_BLK * PHOIT_CACHE_BLK;

/*********************************************************************************************************
 * 函数
*********************************************************************************************************/
BOOL     hoitEnableCache(UINT8 uiCacheBlockSize, UINT8 uiCacheBlockNums);
BOOL     hoitReadFromCache(UINT32 uiOfs, PCHAR pContent, UINT32 uiSize);
BOOL     hoitWriteToCache(UINT32 uiOfs, PCHAR pContent, UINT32 uiSize);
BOOL     hoitFlushCache();

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSCACHE_H_ */
