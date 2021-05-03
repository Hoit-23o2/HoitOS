/*********************************************************************************************************
**
**                                    中国软件??源组??
**
**                                   嵌入式实时操作系??
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------??????--------------------------------------------------------------------------------
**
** ??   ??   ??: hoitCache.h
**
** ??   ??   ??: ??????
**
** ???????????: 2021 ?? 04 ?? 02 ??
**
** ??        ??: ?????
*********************************************************************************************************/

#ifndef SYLIXOS_EXTFS_HOITFS_HOITFSCACHE_H_
#define SYLIXOS_EXTFS_HOITFS_HOITFSCACHE_H_

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
//#define HOIT_CACHE_TEST
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/include/fs_fs.h"

#include "hoitType.h"
#include "hoitFsLib.h"
#include "SylixOS.h"
/*********************************************************************************************************
 * 结构
*********************************************************************************************************/
/* cache Type */
#define HOIT_CACHE_TYPE_INVALID     0
#define HOIT_CACHE_TYPE_DATA        1







/*********************************************************************************************************
 * ????
*********************************************************************************************************/
PHOIT_CACHE_HDR     hoitEnableCache(UINT32 uiCacheBlockSize, 
                                    UINT32 uiCacheBlockNums, 
                                    PHOIT_VOLUME phoitfs);
PHOIT_CACHE_BLK     hoitAllocCache(PHOIT_CACHE_HDR pcacheHdr, 
                                    UINT32 flashBlkNo, 
                                    UINT32 cacheType, 
                                    PHOIT_ERASABLE_SECTOR pSector);

PHOIT_CACHE_BLK     hoitCheckCacheHit(PHOIT_CACHE_HDR pcacheHdr, 
                                        UINT32 flashBlkNo);
BOOL                hoitReadFromCache(PHOIT_CACHE_HDR pcacheHdr, 
                                        UINT32 uiOfs, 
                                        PCHAR pContent, 
                                        UINT32 uiSize);

UINT32      hoitFlushCache(PHOIT_CACHE_HDR pcacheHdr);
UINT32      hoitFindNextToWrite(PHOIT_CACHE_HDR pcacheHdr, UINT32 cacheType);
#ifdef HOIT_CACHE_TEST
BOOL    test_hoit_cache();
BOOL    hoitWriteToCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 uiOfs, PCHAR pContent, UINT32 uiSize);
#else
UINT32      hoitWriteToCache(PHOIT_CACHE_HDR pcacheHdr, PCHAR pContent, UINT32 uiSize);
#endif

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSCACHE_H_ */
