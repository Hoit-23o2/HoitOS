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


#define __HOITFS_CACHE_LOCK(pcacheHdr)          API_SemaphoreMPend(pcacheHdr->HOITCACHE_hLock, \
                                                LW_OPTION_WAIT_INFINITE)
#define __HOITFS_CACHE_UNLOCK(pcacheHdr)        API_SemaphoreMPost(pcacheHdr->HOITCACHE_hLock)
/*
    uiCacheBlockSize        单块cache大小
    uiCacheBlockNums        cache数量
    phoitfs                 hoitfs文件卷结构体
*/
PHOIT_CACHE_HDR hoitEnableCache(UINT32 uiCacheBlockSize, UINT32 uiCacheBlockNums, PHOIT_VOLUME phoitfs){
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

    pcacheHdr->HOITCACHE_hoitfsVol      = phoitfs;
    pcacheHdr->HOITCACHE_blockNums      = 0;
    pcacheHdr->HOITCACHE_blockMaxNums   = uiCacheBlockNums;
    pcacheHdr->HOITCACHE_blockSize      = uiCacheBlockSize;
    pcacheHdr->HOITCACHE_flashBlkNum    = (NOR_FLASH_SZ - NOR_FLASH_START_OFFSET)/uiCacheBlockSize + 1;
    pcacheHdr->HOITCACHE_nextBlkToWrite = 0;
    
    /* 链表头也是一个HOIT_CACHE_BLK，不同在于有效位为0，且HOITBLK_buf为空，以此作为区别 */
    pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_bType            = HOIT_CACHE_TYPE_INVALID;
    pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext    = pcacheHdr->HOITCACHE_cacheLineHdr;
    pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListPrev    = pcacheHdr->HOITCACHE_cacheLineHdr;
    pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_buf              = LW_NULL;

    return pcacheHdr;
}
/*
    分配cache,读取映射在flash的内容，返回成功分配的cache指针
*/
PHOIT_CACHE_BLK hoitAllocCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 flashBlkNo, UINT32 cacheType) {
    INT             i;
    PHOIT_CACHE_BLK pcache;/* 当前创建的cache指针 */
    PHOIT_CACHE_BLK cacheLineHdr = pcacheHdr->HOITCACHE_cacheLineHdr; /* cache链表头指针 */
    size_t          cacheBlkSize = pcacheHdr->HOITCACHE_blockSize;
    BOOL            flag; /* 换块标记 */
    UINT32          blkToWrite;

    if (!pcacheHdr) {
        return LW_NULL;
    }
    
    if (pcacheHdr->HOITCACHE_blockNums == pcacheHdr->HOITCACHE_blockMaxNums) { 
        /* cache分配数量已满，需要换块 */
        flag = LW_TRUE;
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
        flag = LW_FALSE;
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

        pcacheHdr->HOITCACHE_blockNums++;
    }

    if (flag) { 
        /* 换了块要写回 */
        blkToWrite = hoitFindNextToWrite(pcacheHdr, HOIT_CACHE_TYPE_DATA);
        write_nor(blkToWrite*pcacheHdr->HOITCACHE_blockSize + NOR_FLASH_START_OFFSET, 
                    pcache->HOITBLK_buf, 
                    pcacheHdr->HOITCACHE_blockSize, 
                    WRITE_KEEP);
    }

    lib_bzero(pcache->HOITBLK_buf,cacheBlkSize);
    read_nor(flashBlkNo * cacheBlkSize + NOR_FLASH_START_OFFSET, pcache->HOITBLK_buf, cacheBlkSize);

    pcache->HOITBLK_bType           = cacheType;
    pcache->HOITBLK_blkNo           = flashBlkNo;

    return pcache;
}
/* 检测相应flashBlkNo是否命中 */
PHOIT_CACHE_BLK hoitCheckCacheHit(PHOIT_CACHE_HDR pcacheHdr, UINT32 flashBlkNo) {
    PHOIT_CACHE_BLK pcacheLineHdr = pcacheHdr->HOITCACHE_cacheLineHdr;
    PHOIT_CACHE_BLK pcache = pcacheLineHdr->HOITBLK_cacheListNext;
    
    while (pcache != pcacheLineHdr){
        if (pcache->HOITBLK_blkNo == flashBlkNo &&
            pcache->HOITBLK_bType != HOIT_CACHE_TYPE_INVALID) {
            return pcache;
        }
    }
    return LW_NULL;
}

BOOL hoitReadFromCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 uiOfs, PCHAR pContent, UINT32 uiSize){
    PCHAR   pucDest         = pContent;
    size_t  cacheBlkSize    = pcacheHdr->HOITCACHE_blockSize;
    size_t  stStart         = uiOfs % cacheBlkSize;
    PHOIT_CACHE_BLK pcache;
    //UINT32  blkNoStart      = uiOfs/cacheBlkSize;
    //UINT32  blkNoEnd        = (uiOfs + uiSize) / cacheBlkSize;
    UINT32  readBytes      = 0;
    UINT32  i;

    while(uiSize != 0) {
        UINT32  stBufSize = (cacheBlkSize - stStart);
        i = (uiOfs + readBytes)/cacheBlkSize;
        if (stBufSize > uiSize) {
            pcache = hoitCheckCacheHit(pcacheHdr, i);

            if (!pcache) {
                /* 未命中 */
                pcache = hoitAllocCache(pcacheHdr, i, HOIT_CACHE_TYPE_DATA);
                if(!pcache) {
                    /* 分配失败 */
                    read_nor(uiOfs + readBytes + NOR_FLASH_START_OFFSET, pucDest, uiSize);
                }
            } else {
                /* 命中了 */
                lib_memcpy(pContent, pcache->HOITBLK_buf+stStart, uiSize);
            }
            readBytes   += uiSize;
            uiSize       = 0;
        } else {
            pcache = hoitCheckCacheHit(pcacheHdr, i);
            read_nor(uiOfs + readBytes + NOR_FLASH_START_OFFSET, pucDest, stBufSize);
            if (!pcache) {
                /* 未命中 */
                hoitAllocCache(pcacheHdr, i, HOIT_CACHE_TYPE_DATA);
                if(!pcache) {
                    /* 分配失败 */
                    read_nor(uiOfs + readBytes + NOR_FLASH_START_OFFSET, pucDest, stBufSize);
                }                
            } else {
                /* 命中了 */
                lib_memcpy(pContent, pcache->HOITBLK_buf+stStart, uiSize);
            }
            pucDest     += cacheBlkSize;
            uiSize      -= cacheBlkSize;
            readBytes   += cacheBlkSize;
            stStart      = 0;
        }
    }

    return LW_TRUE;
}

BOOL hoitWriteToCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 uiOfs, PCHAR pContent, UINT32 uiSize){
    PCHAR   pucDest         = pContent;
    size_t  cacheBlkSize    = pcacheHdr->HOITCACHE_blockSize;
    size_t  stStart         = uiOfs % cacheBlkSize;
    UINT32  blkNoStart      = uiOfs/cacheBlkSize;
    UINT32  blkNoEnd        = (uiOfs + uiSize) / cacheBlkSize;
    UINT32  writeBytes      = 0;
    UINT32  blkToWrite      = 0; /* 下一块要写入的flash块 */
    UINT32  i;

    PHOIT_CACHE_BLK pcache;

    while(uiSize != 0) {
        size_t  stBufSize = (cacheBlkSize - stStart);
        i = (uiOfs + writeBytes)/cacheBlkSize;
        UINT32  writeAddr = uiOfs + writeBytes + NOR_FLASH_START_OFFSET;
        if (stBufSize > uiSize) {
            //read_nor(stStart + i*cacheBlkSize + NOR_FLASH_START_OFFSET, pucDest, uiSize);
            pcache = hoitCheckCacheHit(pcacheHdr, i);
            if (pcache == LW_NULL) { /* 未命中 */
                pcache = hoitAllocCache(pcacheHdr, i, HOIT_CACHE_TYPE_DATA);
                if (pcache == LW_NULL) { /* 未成功分配cache，直接写入flash */
                    //TODO 尚未知道上层文件系统通知在哪写入当前cache
                    blkToWrite = hoitFindNextToWrite(pcacheHdr, pcache->HOITBLK_bType);
                    write_nor(writeAddr, pContent, uiSize, WRITE_KEEP);
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
                pcache = hoitAllocCache(pcacheHdr, i, HOIT_CACHE_TYPE_DATA);
                if (pcache == LW_NULL) { /* 未成功分配cache，直接写入flash */
                    //TODO 尚未知道上层文件系统通知在哪写入当前cache
                    blkToWrite = hoitFindNextToWrite(pcacheHdr, pcache->HOITBLK_bType);
                    write_nor(writeAddr, pContent, stBufSize, WRITE_KEEP);
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

/* 
    将cache中的数据全部写回flash
    返回写回的块数
*/
UINT32 hoitFlushCache(PHOIT_CACHE_HDR pcacheHdr) {
    PHOIT_CACHE_BLK tempCache;
    UINT32  writeCount = 0;
    UINT32  blkToWrite;
    UINT32  writeAddr;
    if (pcacheHdr->HOITCACHE_blockNums == 0)
        return 0;
    
    tempCache = pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext;
    while (tempCache != pcacheHdr->HOITCACHE_cacheLineHdr) {
        if (tempCache->HOITBLK_bType == HOIT_CACHE_TYPE_INVALID)
            continue;
        //TODO 需要获取写flash的位置
        blkToWrite  = hoitFindNextToWrite(pcacheHdr, tempCache->HOITBLK_bType);
        writeAddr   = tempCache->HOITBLK_blkNo*pcacheHdr->HOITCACHE_blockSize + NOR_FLASH_START_OFFSET;
        write_nor(writeAddr, 
                    tempCache->HOITBLK_buf, 
                    pcacheHdr->HOITCACHE_blockSize, 
                    WRITE_KEEP);

        tempCache = tempCache->HOITBLK_cacheListNext;
        writeCount ++;
    }

    return writeCount;
}
/*
    释放除链表头以外所有HOIT_CACHE_BLK
*/
BOOL hoitReleaseCache(PHOIT_CACHE_HDR pcacheHdr) {
    PHOIT_CACHE_BLK tempCache, tempCachePre;
    tempCache = pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext;
    while (tempCache!= pcacheHdr->HOITCACHE_cacheLineHdr)
    {
        if (tempCache->HOITBLK_buf != LW_NULL) {
            __SHEAP_FREE(tempCache->HOITBLK_buf);
        }
        tempCachePre = tempCache;
        tempCache = tempCache->HOITBLK_cacheListNext;
        __SHEAP_FREE(tempCachePre);
    }
    return 1;
}

/*
    释放cache头
*/
BOOL hoitReleaseCacheHDR(PHOIT_CACHE_HDR pcacheHdr) {
    if (pcacheHdr->HOITCACHE_cacheLineHdr != LW_NULL) {
        __SHEAP_FREE(pcacheHdr->HOITCACHE_cacheLineHdr);
    }
    API_SemaphoreMDelete(&pcacheHdr->HOITCACHE_hLock);
}
/*
    返回下一个要写的块，并更新PHOIT_CACHE_HDR中HOITCACHE_nextBlkToWrite(要写的下一块)
*/
UINT32 hoitFindNextToWrite(PHOIT_CACHE_HDR pcacheHdr, UINT32 cacheType) {
    switch (cacheType)
    {
    case HOIT_CACHE_TYPE_INVALID:
        return (PX_ERROR);
    case HOIT_CACHE_TYPE_DATA:
        pcacheHdr->HOITCACHE_nextBlkToWrite ++;
        return pcacheHdr->HOITCACHE_nextBlkToWrite -1;
    //TODO 将来有了gc之后就不能单纯的将下一个写入块加一
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
}

#ifdef HOIT_CACHE_TEST
BOOL test_hoit_cache() {
    PHOIT_CACHE_HDR pcacheHdr;
    CHAR            data_write[8] = "2233445\0";
    CHAR            data_read[8];
    lib_memset(data_read,0,sizeof(data_read));
    printf("======================  hoit cache test   ============================\n");
    
    printf("1.common write read\n");
    pcacheHdr = hoitEnableCache(64, 8, LW_NULL);
    hoitWriteToCache(pcacheHdr, 0, data_write, sizeof(data_write));
    hoitReadFromCache(pcacheHdr, 4, data_read, sizeof(data_read));
    printf("result: %s\n",data_read);

    printf("2.common flush\n");
    hoitFlushCache(pcacheHdr);
    lib_memset(data_read,0,sizeof(data_read));
    read_nor((pcacheHdr->HOITCACHE_nextBlkToWrite-1)*pcacheHdr->HOITCACHE_blockSize+4+NOR_FLASH_START_OFFSET,data_read,sizeof(data_read));
    printf("result: %s\n",data_read);

    printf("======================  hoit cache test end  =========================\n");
    return LW_TRUE;
}
#endif
