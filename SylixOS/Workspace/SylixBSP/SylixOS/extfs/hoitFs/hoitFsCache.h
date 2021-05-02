/*********************************************************************************************************
**
<<<<<<< HEAD
**                                    ‰∏≠ÂõΩËΩØ‰ª∂ÔøΩ?Ê∫êÁªÑÔøΩ?
**
**                                   ÂµåÂÖ•ÂºèÂÆûÊó∂Êìç‰ΩúÁ≥ªÔøΩ?
=======
**                                    ÷–π˙»Ìº˛ø™‘¥◊È÷Ø
**
**                                   «∂»Î Ω µ ±≤Ÿ◊˜œµÕ≥
>>>>>>> 3d721479faf46c7ab9d923e5b31785af351d8932
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------Œƒº˛–≈œ¢--------------------------------------------------------------------------------
**
<<<<<<< HEAD
** ÔøΩ?   ÔøΩ?   ÔøΩ?: hoitCache.h
**
** ÔøΩ?   ÔøΩ?   ÔøΩ?: ÊΩòÂª∂ÔøΩ?
**
** Êñá‰ª∂ÂàõÂª∫Êó•Êúü: 2021 ÔøΩ? 04 ÔøΩ? 02 ÔøΩ?
**
** ÔøΩ?        ÔøΩ?: ÁºìÂ≠òÔøΩ?
=======
** Œƒ   º˛   √˚: hoitCache.h
**
** ¥¥   Ω®   »À: ≈À—”˜Ë
**
** Œƒº˛¥¥Ω®»’∆⁄: 2021 ƒÍ 04 ‘¬ 02 »’
**
** √Ë         ˆ: ª∫¥Ê≤„
>>>>>>> 3d721479faf46c7ab9d923e5b31785af351d8932
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
<<<<<<< HEAD
 * ÁªìÊûÑ
*********************************************************************************************************/
/* cache Type */
#define HOIT_CACHE_TYPE_INVALID     0
#define HOIT_CACHE_TYPE_DATA        1
=======
 * Ω·ππÃÂ
*********************************************************************************************************/

typedef struct HOIT_CACHE_HDR
{
    UINT8               HOITCACHE_blockSize;    /* µ•∏ˆcache¥Û–° */
    UINT8               HOITCACHE_blockNums;    /* cache◊Ó¥Û ˝¡ø */
    LW_OBJECT_HANDLE    HOITCACHE_hVolLock;     /* cache◊‘–˝À¯ */
}HOIT_CACHE_HDR;
typedef HOIT_CACHE_HDR * PHOIT_CACHE_HDR;
>>>>>>> 3d721479faf46c7ab9d923e5b31785af351d8932

typedef struct HOIT_CACHE_BLK
{
    BOOL                        HOITBLK_bType;          /* cacheÂùóÁ±ªÂûãÔºå‰∏∫0 */
    UINT32                      HOITBLK_blkNo;          /* cacheÂùóÂè∑ */
    struct HOIT_CACHE_BLK       *HOITBLK_cacheListPrev;  /* ÈìæË°®‰∏ä‰∏ä‰∏Ä‰∏™cache */
    struct HOIT_CACHE_BLK       *HOITBLK_cacheListNext;  /* ÈìæË°®‰∏ä‰∏ã‰∏Ä‰∏™cache */
    PCHAR                       HOITBLK_buf;            /* Êï∞ÊçÆÔøΩ? */
}HOIT_CACHE_BLK;
typedef HOIT_CACHE_BLK * PHOIT_CACHE_BLK;

typedef struct HOIT_CACHE_HDR
{
    PHOIT_VOLUME            HOITCACHE_hoitfsVol;
    size_t                  HOITCACHE_blockSize;    /* Âçï‰∏™cacheÂ§ßÂ∞è */
    UINT32                  HOITCACHE_blockMaxNums; /* cacheÊúÄÂ§ßÊï∞Èáè */
    UINT32                  HOITCACHE_blockNums;    /* ÂΩìÂâçcacheÊï∞Èáè */
    LW_OBJECT_HANDLE        HOITCACHE_hLock;        /* cacheËá™ÊóãÈîÅ? */
    UINT32                  HOITCACHE_flashBlkNum;  /* Â∞ÜflashÂàÜÂùóÂêéÁöÑÂùóÊï∞ */
    PHOIT_CACHE_BLK         HOITCACHE_cacheLineHdr;  /* cacheÈìæË°® */
    UINT32                  HOITCACHE_nextBlkToWrite;/* ‰∏ã‰∏Ä‰∏™Ë¶ÅËæìÂá∫ÁöÑÂùó */
}HOIT_CACHE_HDR;
typedef HOIT_CACHE_HDR * PHOIT_CACHE_HDR;

/*********************************************************************************************************
 * ∫Ø ˝
*********************************************************************************************************/
PHOIT_CACHE_HDR     hoitEnableCache(UINT32 uiCacheBlockSize, UINT32 uiCacheBlockNums, PHOIT_VOLUME phoitfs);
PHOIT_CACHE_BLK hoitAllocCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 flashBlkNo, UINT32 cacheType);
PHOIT_CACHE_BLK     hoitCheckCacheHit(PHOIT_CACHE_HDR pcacheHdr, UINT32 flashBlkNo);
BOOL    hoitReadFromCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 uiOfs, PCHAR pContent, UINT32 uiSize);
BOOL    hoitWriteToCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 uiOfs, PCHAR pContent, UINT32 uiSize);
UINT32  hoitFlushCache(PHOIT_CACHE_HDR pcacheHdr);
UINT32  hoitFindNextToWrite(PHOIT_CACHE_HDR pcacheHdr, UINT32 cacheType);
#ifdef HOIT_CACHE_TEST
BOOL test_hoit_cache();
#endif

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSCACHE_H_ */
