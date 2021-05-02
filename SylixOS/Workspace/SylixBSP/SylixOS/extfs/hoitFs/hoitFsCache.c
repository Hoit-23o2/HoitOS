/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: hoitCache.c
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 04 �� 02 ��
**
** ��        ��: �����
*********************************************************************************************************/
#include "hoitFsCache.h"
#include "driver/mtd/nor/nor.h"
BOOL hoitEnableCache(UINT8 uiCacheBlockSize, UINT8 uiCacheBlockNums){
    
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

BOOL hoitReadFromCache(UINT32 uiOfs, PCHAR pContent, UINT32 uiSize){
    read_nor(uiOfs, pContent, uiSize);
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
