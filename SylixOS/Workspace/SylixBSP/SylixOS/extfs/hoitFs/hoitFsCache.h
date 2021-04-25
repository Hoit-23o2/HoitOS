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

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/include/fs_fs.h"

#include "hoitType.h"
#include "SylixOS.h"
/*********************************************************************************************************
 * 结构体
*********************************************************************************************************/

typedef struct HOIT_CACHE_HDR
{
    size_t              HOITCACHE_blockSize;    /* 单个cache大小 */
    UINT32              HOITCACHE_blockMaxNums; /* cache最大数量 */
    UINT32              HOITCACHE_blockNums;    /* 当前cache数量 */
    LW_OBJECT_HANDLE    HOITCACHE_hLock;        /* cache自旋锁 */
    UINT32              HOITCACHE_flashBlkNum;  /* 将flash分块后的块数量 */
    PHOIT_CACHE_BLK     HOITCACHE_cacheLineHdr;  /* cache链表头 */
}HOIT_CACHE_HDR;
typedef HOIT_CACHE_HDR * PHOIT_CACHE_HDR;

typedef struct HOIT_CACHE_BLK
{
    BOOL                HOITBLK_bValid;         /* 链表有效位 */
    UINT32              HOITBLK_blkNo;          /* cache块号 */
    PHOIT_CACHE_BLK     HOITBLK_cacheListPrev;  /* 链表上上一个cache */
    PHOIT_CACHE_BLK     HOITBLK_cacheListNext;  /* 链表上下一个cache */
    PCHAR               HOITBLK_buf;            /* 数据区 */
}HOIT_CACHE_BLK;
typedef HOIT_CACHE_BLK * PHOIT_CACHE_BLK;

/*********************************************************************************************************
 * 函数
*********************************************************************************************************/
PHOIT_CACHE_HDR     hoitEnableCache(UINT32 uiCacheBlockSize, UINT32 uiCacheBlockNums);
PHOIT_CACHE_BLK     hoitAllocCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 flashBlkNo);
PHOIT_CACHE_BLK     hoitCheckCacheHit(PHOIT_CACHE_HDR pcacheHdr, UINT32 flashBlkNo);
BOOL    hoitReadFromCache(PHOIT_CACHE_HDR pcacheHdr, size_t uiOfs, PCHAR pContent, size_t uiSize);
BOOL    hoitWriteToCache(UINT32 uiOfs, PCHAR pContent, UINT32 uiSize);
BOOL    hoitFlushCache();

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSCACHE_H_ */
