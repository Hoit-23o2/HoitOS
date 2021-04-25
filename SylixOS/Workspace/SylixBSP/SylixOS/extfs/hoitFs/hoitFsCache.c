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
** 文   件   名: hoitCache.c
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 04 月 02 日
**
** 描        述: 缓存层
*********************************************************************************************************/
#include "hoitFsCache.h"
#include "../../driver/mtd/nor/nor.h"
/*********************************************************************************************************
** 函数名称: hoitEnableCache
** 功能描述: 初始化 hoit cache
** 输　入  : uiCacheBlockSize       单个cache大小
**           uiCacheBlockNums       cache最大数量
** 输　出  : LW_NULL 表示失败，PHOIT_CACHE_HDR地址 表示成功
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
PHOIT_CACHE_HDR hoitEnableCache(UINT32 uiCacheBlockSize, UINT32 uiCacheBlockNums){
    PHOIT_CACHE_HDR pcacheHdr;

    pcacheHdr = (PHOIT_CACHE_HDR)__SHEAP_ALLOC(sizeof(HOIT_CACHE_HDR));
    if (pcacheHdr == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);        
    }

    lib_bzero(pcacheHdr,sizeof(HOIT_CACHE_HDR));

    pcacheHdr->HOITCACHE_hLock = API_SemaphoreMCreate("hoit_cache_lock", LW_PRIO_DEF_CEILING,
                                             LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                                             LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                             LW_NULL);

    if (!pcacheHdr->HOITCACHE_hLock) {
        __SHEAP_FREE(pcacheHdr);
        return  (LW_NULL);
    }

    pcacheHdr->HOITCACHE_cacheLineHdr    = (PHOIT_CACHE_BLK)__SHEAP_ALLOC(sizeof(HOIT_CACHE_BLK));
    if (pcacheHdr == LW_NULL) {
        __SHEAP_FREE(pcacheHdr);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (LW_NULL);        
    }

    pcacheHdr->HOITCACHE_blockNums      = 0;
    pcacheHdr->HOITCACHE_blockMaxNums   = uiCacheBlockNums;
    pcacheHdr->HOITCACHE_blockSize      = uiCacheBlockSize;
    pcacheHdr->HOITCACHE_flashBlkNum    = (NOR_FLASH_SZ - NOR_FLASH_START_OFFSET)/uiCacheBlockSize + 1;
    
    /* 链表头也是一个HOIT_CACHE_BLK，不同在于有效位为0，且HOITBLK_buf为空，以此作为区别 */
    pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_bValid            = LW_FALSE;
    pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext     = pcacheHdr->HOITCACHE_cacheLineHdr;
    pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListPrev     = pcacheHdr->HOITCACHE_cacheLineHdr;
    pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_buf               = LW_NULL;

    return pcacheHdr;
}
/*
    分配cache,读取映射在flash的内容，返回成功分配的cache指针
*/
PHOIT_CACHE_BLK hoitAllocCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 flashBlkNo) {
    INT             i;
    PHOIT_CACHE_BLK pcache;/* 当前创建的cache指针 */
    PHOIT_CACHE_BLK cacheLineHdr = pcacheHdr->HOITCACHE_cacheLineHdr; /* cache链表头指针 */
    size_t  cacheBlkSize    = pcacheHdr->HOITCACHE_blockSize;
    if (!pcacheHdr) {
        return LW_NULL;
    }
    
    if (pcacheHdr->HOITCACHE_blockNums == pcacheHdr->HOITCACHE_blockMaxNums) { 
        /* cache分配数量已满，需要换块 */
        //TOOPT 换块算法暂时采用FIFO
        pcache = cacheLineHdr->HOITBLK_cacheListPrev;

        /* 从链表尾部断开 */
        cacheLineHdr->HOITBLK_cacheListPrev = pcache->HOITBLK_cacheListPrev;
        pcache->HOITBLK_cacheListPrev->HOITBLK_cacheListNext = pcache->HOITBLK_cacheListNext;

        /* 直接插入链表头部 */
        pcache->HOITBLK_cacheListPrev   = cacheLineHdr;
        pcache->HOITBLK_cacheListNext   = cacheLineHdr->HOITBLK_cacheListNext;
        cacheLineHdr->HOITBLK_cacheListNext->HOITBLK_cacheListPrev  = pcache;
        cacheLineHdr->HOITBLK_cacheListNext     = pcache;
    } else {
        pcache = (PHOIT_CACHE_BLK)__SHEAP_ALLOC(sizeof(HOIT_CACHE_BLK));
        if (pcache == NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            return i;
        }

        pcache->HOITBLK_buf = (PCHAR)__SHEAP_ALLOC(cacheBlkSize);
        if (pcache == NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            return i;
        }

        /* 插入新分配的cache */
        pcache->HOITBLK_cacheListPrev   = cacheLineHdr;
        pcache->HOITBLK_cacheListNext   = cacheLineHdr->HOITBLK_cacheListNext;
        cacheLineHdr->HOITBLK_cacheListNext->HOITBLK_cacheListPrev  = pcache;
        cacheLineHdr->HOITBLK_cacheListNext     = pcache;
    }



    lib_bzero(pcache->HOITBLK_buf,cacheBlkSize);
    read_nor(flashBlkNo * cacheBlkSize + NOR_FLASH_START_OFFSET, pcache->HOITBLK_buf, cacheBlkSize);

    pcache->HOITBLK_bValid          = LW_TRUE;
    pcache->HOITBLK_blkNo           = flashBlkNo;




    return pcache;
}
/* 检测相应flashBlkNo是否命中 */
PHOIT_CACHE_BLK hoitCheckCacheHit(PHOIT_CACHE_HDR pcacheHdr, UINT32 flashBlkNo) {
    PHOIT_CACHE_BLK pcacheLineHdr = pcacheHdr->HOITCACHE_cacheLineHdr;
    PHOIT_CACHE_BLK pcache = pcacheLineHdr->HOITBLK_cacheListNext;
    
    do {
        if (pcache->HOITBLK_blkNo == flashBlkNo) {
            return pcache;
        }
    }while (pcache != pcacheLineHdr);
    return LW_NULL;
}

BOOL hoitReadFromCache(PHOIT_CACHE_HDR pcacheHdr, size_t uiOfs, PCHAR pContent, size_t uiSize){
    PCHAR   pucDest         = pContent;
    size_t  cacheBlkSize    = pcacheHdr->HOITCACHE_blockSize;
    size_t  stStart         = uiOfs % cacheBlkSize;
    UINT32  blkNoStart      = uiOfs/cacheBlkSize;
    UINT32  blkNoEnd        = (uiOfs + uiSize) / cacheBlkSize;
    UINT32  i;

    for (i = blkNoStart; i<=blkNoEnd; i++) {
        size_t  stBufSize = (cacheBlkSize - stStart);
        if (stBufSize > uiSize) {
            read_nor(stStart + i*cacheBlkSize + NOR_FLASH_START_OFFSET, pucDest, uiSize);
            if (!hoitCheckCacheHit(pcacheHdr, i)) {
                hoitAllocCache(pcacheHdr, i);
            }
            uiSize = 0;
            break;
        } else {
            read_nor(stStart + i*cacheBlkSize + NOR_FLASH_START_OFFSET, pucDest, stBufSize);
            if (!hoitCheckCacheHit(pcacheHdr, i)) {
                hoitAllocCache(pcacheHdr, i);
            }

            pucDest += cacheBlkSize;
            uiSize  -= cacheBlkSize;
            stStart  = 0;
        }
    }
    // do {
    //     if (cacheBlkSize >= uiSize) { /*  */
    //         read_nor(stStart, pucDest, uiSize);
    //         flashBlkNo = stStart/cacheBlkSize;
    //         if (!hoitCheckCacheHit(pcacheHdr, flashBlkNo)) {
    //             hoitAllocCache(pcacheHdr, flashBlkNo);
    //         }
    //         break;
    //     } else {
    //         read_nor(stStart, pucDest, cacheBlkSize);
    //         flashBlkNo = (stStart)
    //         if (!hoitCheckCacheHit(pcacheHdr, flashBlkNo)) {
    //             hoitAllocCache(pcacheHdr, flashBlkNo);
    //         }
    //         break;
    //     }
    // }while (uiSize);
    return LW_TRUE;
}

BOOL hoitWriteToCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 uiOfs, PCHAR pContent, UINT32 uiSize){
    PCHAR   pucDest         = pContent;
    size_t  cacheBlkSize    = pcacheHdr->HOITCACHE_blockSize;
    size_t  stStart         = uiOfs % cacheBlkSize;
    UINT32  blkNoStart      = uiOfs/cacheBlkSize;
    UINT32  blkNoEnd        = (uiOfs + uiSize) / cacheBlkSize;
    UINT32  i;

    PHOIT_CACHE_BLK pcache;

    for (i = blkNoStart; i<=blkNoEnd; i++) {
        size_t  stBufSize = (cacheBlkSize - stStart);
        if (stBufSize > uiSize) {
            //read_nor(stStart + i*cacheBlkSize + NOR_FLASH_START_OFFSET, pucDest, uiSize);
            pcache = hoitCheckCacheHit(pcacheHdr, i);
            if (pcache == LW_NULL) { /* 未命中 */
                pcache = hoitAllocCache(pcacheHdr, i);
                if (pcache != LW_NULL) { /* 未成功分配cache，直接写入flash */
                    //TODO 尚未知道上层文件系统通知在哪写入当前cache
                }
                else { /* 成功分配cache，则写入cache */
                    lib_memcpy(pcache->HOITBLK_buf + stStart, pucDest, uiSize);
                }
            } else {
                lib_memcpy(pcache->HOITBLK_buf + stStart, pucDest, uiSize);
            }
            uiSize = 0;
            break;
        } else {
            //read_nor(stStart + i*cacheBlkSize + NOR_FLASH_START_OFFSET, pucDest, stBufSize);
            pcache = hoitCheckCacheHit(pcacheHdr, i);
            if (pcache == LW_NULL) { /* 未命中 */
                pcache = hoitAllocCache(pcacheHdr, i);
                if (pcache != LW_NULL) { /* 未成功分配cache，直接写入flash */
                    //TODO 尚未知道上层文件系统通知在哪写入当前cache
                }
                else { /* 成功分配cache，则写入cache */
                    lib_memcpy(pcache->HOITBLK_buf + stStart, pucDest, cacheBlkSize);
                }
            } else {
                lib_memcpy(pcache->HOITBLK_buf + stStart, pucDest, cacheBlkSize);
            }

            pucDest += cacheBlkSize;
            uiSize  -= cacheBlkSize;
            stStart  = 0;
        }
    }
    return LW_TRUE;   
}