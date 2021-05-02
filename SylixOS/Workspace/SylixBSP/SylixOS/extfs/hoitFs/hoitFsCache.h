/*********************************************************************************************************
**
<<<<<<< HEAD
**                                    �й����??Դ��??
**
**                                   Ƕ��ʽʵʱ����ϵ??
=======
**                                    ?��???????????
**
**                                   ????????????
>>>>>>> 3d721479faf46c7ab9d923e5b31785af351d8932
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
#define HOIT_CACHE_TEST
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/include/fs_fs.h"

#include "hoitType.h"
#include "hoitFsLib.h"
#include "SylixOS.h"
/*********************************************************************************************************
 * �ṹ
*********************************************************************************************************/
/* cache Type */
#define HOIT_CACHE_TYPE_INVALID     0
#define HOIT_CACHE_TYPE_DATA        1

typedef struct HOIT_CACHE_BLK
{
    BOOL                        HOITBLK_bType;          /* cache�����ͣ�Ϊ0 */
    UINT32                      HOITBLK_blkNo;          /* cache��� */
    struct HOIT_CACHE_BLK       *HOITBLK_cacheListPrev;  /* ��������һ��cache */
    struct HOIT_CACHE_BLK       *HOITBLK_cacheListNext;  /* ��������һ��cache */
    PCHAR                       HOITBLK_buf;            /* ����?? */
}HOIT_CACHE_BLK;
typedef HOIT_CACHE_BLK * PHOIT_CACHE_BLK;

typedef struct HOIT_CACHE_HDR
{
    PHOIT_VOLUME            HOITCACHE_hoitfsVol;
    size_t                  HOITCACHE_blockSize;    /* ����cache��С */
    UINT32                  HOITCACHE_blockMaxNums; /* cache������� */
    UINT32                  HOITCACHE_blockNums;    /* ��ǰcache���� */
    LW_OBJECT_HANDLE        HOITCACHE_hLock;        /* cache������? */
    UINT32                  HOITCACHE_flashBlkNum;  /* ��flash�ֿ��Ŀ��� */
    PHOIT_CACHE_BLK         HOITCACHE_cacheLineHdr;  /* cache���� */
    UINT32                  HOITCACHE_nextBlkToWrite;/* ��һ��Ҫ����Ŀ� */
}HOIT_CACHE_HDR;
typedef HOIT_CACHE_HDR * PHOIT_CACHE_HDR;

/*********************************************************************************************************
 * ????
*********************************************************************************************************/
PHOIT_CACHE_HDR     hoitEnableCache(UINT32 uiCacheBlockSize, UINT32 uiCacheBlockNums, PHOIT_VOLUME phoitfs);
PHOIT_CACHE_BLK     hoitAllocCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 flashBlkNo, UINT32 cacheType);
PHOIT_CACHE_BLK     hoitCheckCacheHit(PHOIT_CACHE_HDR pcacheHdr, UINT32 flashBlkNo);
BOOL    hoitReadFromCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 uiOfs, PCHAR pContent, UINT32 uiSize);
BOOL    hoitWriteToCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 uiOfs, PCHAR pContent, UINT32 uiSize);
UINT32  hoitFlushCache(PHOIT_CACHE_HDR pcacheHdr);
UINT32  hoitFindNextToWrite(PHOIT_CACHE_HDR pcacheHdr, UINT32 cacheType);
#ifdef HOIT_CACHE_TEST
BOOL test_hoit_cache();
#endif

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSCACHE_H_ */
