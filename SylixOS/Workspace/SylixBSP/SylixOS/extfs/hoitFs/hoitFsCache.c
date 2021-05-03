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
#include "../../driver/mtd/nor/nor.h"
/*********************************************************************************************************
** ��������: hoitEnableCache
** ��������: ��ʼ�� hoit cache
** �䡡��  : uiCacheBlockSize       ����cache��С
**           uiCacheBlockNums       cache�������
** �䡡��  : LW_NULL ��ʾʧ�ܣ�PHOIT_CACHE_HDR��ַ ��ʾ�ɹ�
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/


#define __HOITFS_CACHE_LOCK(pcacheHdr)          API_SemaphoreMPend(pcacheHdr->HOITCACHE_hLock, \
                                                LW_OPTION_WAIT_INFINITE)
#define __HOITFS_CACHE_UNLOCK(pcacheHdr)        API_SemaphoreMPost(pcacheHdr->HOITCACHE_hLock)
/*    
** ��������:    hoitEnableCache
** ��������:    ��ʼ�� hoit cache
** �䡡��  :    uiCacheBlockSize       ����cache��С
**              uiCacheBlockNums       cache�������
**              phoitfs                hoitfs�ļ����ṹ��
** �䡡��  : LW_NULL ��ʾʧ�ܣ�PHOIT_CACHE_HDR��ַ ��ʾ�ɹ�
** ȫ�ֱ���:
** ����ģ��:    
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
    
    /* ����ͷҲ��һ��HOIT_CACHE_BLK����ͬ������ЧλΪ0����HOITBLK_bufΪ�գ��Դ���Ϊ���� */
    pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_bType            = HOIT_CACHE_TYPE_INVALID;
    pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext    = pcacheHdr->HOITCACHE_cacheLineHdr;
    pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListPrev    = pcacheHdr->HOITCACHE_cacheLineHdr;
    pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_buf              = LW_NULL;

    phoitfs->HOITFS_cacheHdr = pcacheHdr;
    return pcacheHdr;
}
/*    
** ��������:    hoitAllocCache
** ��������:    ����cache,��ȡӳ����flash������
** �䡡��  :    pcacheHdr               cacheͷ�ṹ
**              flashBlkNo              ����cacheӳ����������
**              cacheType               cache�е��������ͣ���ʱ��δ�õ�
**              pSector                 cache��Ӧerasable_sector_list�е�sector��
** �䡡��  : LW_NULL ��ʾʧ�ܣ��ɹ�ʱ���ط����cacheָ��
** ȫ�ֱ���:
** ����ģ��:    
*/
PHOIT_CACHE_BLK hoitAllocCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 flashBlkNo, UINT32 cacheType, PHOIT_ERASABLE_SECTOR pSector) {
    INT             i;
    PHOIT_CACHE_BLK pcache;/* ��ǰ������cacheָ�� */
    PHOIT_CACHE_BLK cacheLineHdr = pcacheHdr->HOITCACHE_cacheLineHdr; /* cache����ͷָ�� */
    size_t          cacheBlkSize = pcacheHdr->HOITCACHE_blockSize;
    BOOL            flag; /* ������ */
    UINT32          blkToWrite;

    if (!pcacheHdr || !pSector) {
        return LW_NULL;
    }
    
    if (pcacheHdr->HOITCACHE_blockNums == pcacheHdr->HOITCACHE_blockMaxNums) { 
        /* cache����������������Ҫ���� */
        flag = LW_TRUE;
        //TOOPT �����㷨��ʱ����FIFO
        pcache = cacheLineHdr->HOITBLK_cacheListPrev;

        /* ������β���Ͽ� */
        cacheLineHdr->HOITBLK_cacheListPrev = pcache->HOITBLK_cacheListPrev;
        pcache->HOITBLK_cacheListPrev->HOITBLK_cacheListNext = pcache->HOITBLK_cacheListNext;

        /* ֱ�Ӳ�������ͷ�� */
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

        /* �����·����cache */
        pcache->HOITBLK_cacheListPrev   = cacheLineHdr;
        pcache->HOITBLK_cacheListNext   = cacheLineHdr->HOITBLK_cacheListNext;
        cacheLineHdr->HOITBLK_cacheListNext->HOITBLK_cacheListPrev  = pcache;
        cacheLineHdr->HOITBLK_cacheListNext     = pcache;

        pcacheHdr->HOITCACHE_blockNums++;
    }

    if (flag) { 
        /* ���˿�Ҫд�� */
        blkToWrite = hoitFindNextToWrite(pcacheHdr, HOIT_CACHE_TYPE_DATA);
        write_nor(pcache->HOITBLK_blkNo*pcacheHdr->HOITCACHE_blockSize + NOR_FLASH_START_OFFSET,
                    pcache->HOITBLK_buf, 
                    pcacheHdr->HOITCACHE_blockSize, 
                    WRITE_KEEP);
    }

    lib_bzero(pcache->HOITBLK_buf,cacheBlkSize);
    read_nor(flashBlkNo * cacheBlkSize + NOR_FLASH_START_OFFSET, pcache->HOITBLK_buf, cacheBlkSize);

    pcache->HOITBLK_bType           = cacheType;
    pcache->HOITBLK_sector          = pSector;
    pcache->HOITBLK_blkNo           = flashBlkNo;

    return pcache;
}
/*    
** ��������:    hoitCheckCacheHit
** ��������:    �����ӦflashBlkNo�Ƿ�����
** �䡡��  :    pcacheHdr               cacheͷ�ṹ
**              flashBlkNo              �������
** �䡡��  : LW_NULL ��ʾʧ�ܣ��ɹ�ʱ�����ڴ������е�cacheָ��
** ȫ�ֱ���:
** ����ģ��:    
*/
PHOIT_CACHE_BLK hoitCheckCacheHit(PHOIT_CACHE_HDR pcacheHdr, UINT32 flashBlkNo) {
    PHOIT_CACHE_BLK pcacheLineHdr = pcacheHdr->HOITCACHE_cacheLineHdr;
    PHOIT_CACHE_BLK pcache = pcacheLineHdr->HOITBLK_cacheListNext;
    
    while (pcache != pcacheLineHdr){
        if (pcache->HOITBLK_blkNo == flashBlkNo &&
            pcache->HOITBLK_bType != HOIT_CACHE_TYPE_INVALID) {
            return pcache;
        }
        pcache = pcache->HOITBLK_cacheListNext;
    }
    return LW_NULL;
}
/*    
** ��������:    hoitReadFromCache
** ��������:    ��ȡflash���ݣ����ȴ��ڴ��ж�ȡ
** �䡡��  :    pcacheHdr               cacheͷ�ṹ
**              uiOfs                   ���ݶ�ȡ��ʼλ��
**              pContent                Ŀ�ĵ�ַ
**              uiSize                  ��ȡ�ֽ�
** �䡡��  : LW_FALSE ��ʾʧ�ܣ�LW_TRUE���سɹ�����ʱ����TRUE��
** ȫ�ֱ���:
** ����ģ��:    
*/
BOOL hoitReadFromCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 uiOfs, PCHAR pContent, UINT32 uiSize){
    // read_nor(uiOfs, pContent, uiSize);
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
                read_nor(uiOfs + readBytes + NOR_FLASH_START_OFFSET, pucDest, uiSize);
           } else {
               /* ������ */
               lib_memcpy(pContent, pcache->HOITBLK_buf+stStart, uiSize);
           }
           readBytes   += uiSize;
           uiSize       = 0;
       } else {
           pcache = hoitCheckCacheHit(pcacheHdr, i);
           read_nor(uiOfs + readBytes + NOR_FLASH_START_OFFSET, pucDest, stBufSize);
           if (!pcache) {
                read_nor(uiOfs + readBytes + NOR_FLASH_START_OFFSET, pucDest, stBufSize);
           } else {
               /* ������ */
               lib_memcpy(pContent, pcache->HOITBLK_buf+stStart, uiSize);
           }
           pucDest     += stBufSize;
           uiSize      -= stBufSize;
           readBytes   += stBufSize;
           stStart      = 0;
       }
   }

    return LW_TRUE;
}
/*    
** ��������:    hoitWriteThroughCache
** ��������:    ��ȡflash���ݣ����ȴ��ڴ��ж�ȡ
** �䡡��  :    pcacheHdr               cacheͷ�ṹ
**              uiOfs                   ����д����ʼλ��
**              pContent                ԭ��ַ
**              uiSize                  д���ֽ�
** �䡡��  : LW_FALSE ��ʾʧ�ܣ�LW_TRUE���سɹ�����ʱ����TRUE��
** ȫ�ֱ���:
** ����ģ��:    
*/
BOOL hoitWriteThroughCache(PHOIT_CACHE_HDR pcacheHdr, UINT32 uiOfs, PCHAR pContent, UINT32 uiSize){
    PCHAR   pucDest         = pContent;
    size_t  cacheBlkSize    = pcacheHdr->HOITCACHE_blockSize;
    size_t  stStart         = uiOfs % cacheBlkSize;
    UINT32  blkNoStart      = uiOfs/cacheBlkSize;
    UINT32  blkNoEnd        = (uiOfs + uiSize) / cacheBlkSize;
    UINT32  writeBytes      = 0;
    UINT32  blkToWrite      = 0; /* ��һ��Ҫд���flash�� */
    UINT32  i;

    PHOIT_CACHE_BLK         pcache;
    PHOIT_ERASABLE_SECTOR   pSector = pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_erasableSectorList;

    while(uiSize != 0) {
        size_t  stBufSize = (cacheBlkSize - stStart);
        i = (uiOfs + writeBytes)/cacheBlkSize;
        UINT32  writeAddr = uiOfs + writeBytes + NOR_FLASH_START_OFFSET;

        while (pSector != LW_NULL ) {
            if (pSector->HOITS_bno != i)
                pSector = pSector->HOITS_next;
        }
        
        pcache = hoitCheckCacheHit(pcacheHdr, i);
        if (stBufSize > uiSize) {
            if (pcache == LW_NULL) { /* δ���� */
                //TODO pSector��ô��ȡ
                pcache = hoitAllocCache(pcacheHdr, i, HOIT_CACHE_TYPE_DATA, pSector);
                if (pcache == LW_NULL) { /* δ�ɹ�����cache��ֱ��д��flash */                      
                    write_nor(writeAddr, pContent, uiSize, WRITE_KEEP);
                }
                else { /* �ɹ�����cache����д��cache */
                    lib_memcpy(pcache->HOITBLK_buf + stStart, pucDest, uiSize);
                }
            } else {
                lib_memcpy(pcache->HOITBLK_buf + stStart, pucDest, uiSize);
            }
            uiSize = 0;
            break;
        } else {
            if (pcache == LW_NULL) { /* δ���� */
                pcache = hoitAllocCache(pcacheHdr, i, HOIT_CACHE_TYPE_DATA, pSector);
                if (pcache == LW_NULL) { /* δ�ɹ�����cache��ֱ��д��flash */
                    //TODO ��δ֪���ϲ��ļ�ϵͳ֪ͨ����д�뵱ǰcache
                    write_nor(writeAddr, pContent, stBufSize, WRITE_KEEP);
                }
                else { /* �ɹ�����cache����д��cache */
                    lib_memcpy(pcache->HOITBLK_buf + stStart, pucDest, stBufSize);
                }
            } else {
                lib_memcpy(pcache->HOITBLK_buf + stStart, pucDest, stBufSize);
            }

            pucDest += stBufSize;
            uiSize  -= stBufSize;
            stStart  = 0;
        }
    }

    return LW_TRUE;   
}

/*    
** ��������:    hoitWriteToCache
** ��������:    ��ȡflash���ݣ����ȴ��ڴ��ж�ȡ
** �䡡��  :    pcacheHdr               cacheͷ�ṹ
**              pContent                ԭ��ַ
**              uiSize                  д���ֽڣ�����ܳ���һ��cache���С
** �䡡��  : �ɹ�ʱ����д���׵�ַ����NOR_FLASH_START_OFFSETΪ0��ַ����
**          ʧ�ܷ���PX_ERROR
** ȫ�ֱ���:
** ����ģ��:    
*/
UINT32 hoitWriteToCache(PHOIT_CACHE_HDR pcacheHdr, PCHAR pContent, UINT32 uiSize){
    PCHAR   pucDest         = pContent;
    UINT32  writeAddr       = 0;
    UINT32  i;

    PHOIT_CACHE_BLK         pcache;
    PHOIT_ERASABLE_SECTOR   pSector = pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_now_sector;    /* ��ǰд��� */

    if (pSector == LW_NULL) {
        return PX_ERROR;
    }
    
    if (uiSize > pcacheHdr->HOITCACHE_blockSize) {
        return PX_ERROR;
    }

    if (pSector->HOITS_uiFreeSize < uiSize) {
        pSector = pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_erasableSectorList;
    }

    while (pSector != LW_NULL) {
        if (pSector->HOITS_uiFreeSize >= uiSize) {
            break;
        }
        pSector = pSector->HOITS_next;
    }

    if (pSector == LW_NULL) {                                   /* flash�ռ����岻�� */
        return PX_ERROR;
    }

    writeAddr = pSector->HOITS_bno * 
                pcacheHdr->HOITCACHE_blockSize + 
                pSector->HOITS_offset + 
                NOR_FLASH_START_OFFSET;
    pcache = hoitCheckCacheHit(pcacheHdr, pSector->HOITS_bno);
    if (pcache == LW_NULL) {                                    /* δ���� */
        pcache = hoitAllocCache(pcacheHdr, pSector->HOITS_bno, HOIT_CACHE_TYPE_DATA, pSector);
        if (pcache == LW_NULL) {                                /* δ�ɹ�����cache��ֱ��д��flash */                        
            write_nor(writeAddr, pContent, uiSize, WRITE_KEEP);
        }
        else {                                                  /* �ɹ�����cache����д��cache */
            lib_memcpy(pcache->HOITBLK_buf + pSector->HOITS_offset, pucDest, uiSize);
        }
    } else {
        lib_memcpy(pcache->HOITBLK_buf + pSector->HOITS_offset, pucDest, uiSize);
    }

    /* ����HOITFS_now_sector */
    pSector->HOITS_offset += uiSize;
    pSector = pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_erasableSectorList;
    while (pSector != LW_NULL) {
        if (pSector->HOITS_uiFreeSize != 0) {
            pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_now_sector = pSector;
            break;
        }
        pSector = pSector->HOITS_next;
    }
    pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_totalUsedSize += uiSize;
    return writeAddr - NOR_FLASH_START_OFFSET;
}

/*

*/
VOID hoitCheckCacheList(PHOIT_CACHE_HDR pcacheHdr) {
    UINT32  i = 0;
    printf("======================  hoit cache check   ===========================\n");

    if (pcacheHdr->HOITCACHE_blockNums == 0) {
        printf("No block in cache\n");
        return ;
    } else {
        PHOIT_CACHE_BLK tempCache = pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext;
        printf("Cache block Size:%d",pcacheHdr->HOITCACHE_blockSize);
        while (tempCache != LW_NULL) {
            i++;
            printf("Cache No.%d\n",i);
            printf("Cache Block Number: %d\n",tempCache->HOITBLK_blkNo);
            printf("Cache Free Size: %d\n",tempCache->HOITBLK_sector->HOITS_uiFreeSize);
        }
    }
    
}

/*    
** ��������:    hoitFlushCache
** ��������:    ������cache����д��flash
** �䡡��  :    pcacheHdr               cacheͷ�ṹ
** �䡡��  :    д�ص�cache������
** ȫ�ֱ���:
** ����ģ��:    
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
        if (tempCache->HOITBLK_sector == LW_NULL)
            continue;
        //TODO ��Ҫ��ȡдflash��λ��
        blkToWrite  = hoitFindNextToWrite(pcacheHdr, tempCache->HOITBLK_bType);
        writeAddr   = tempCache->HOITBLK_blkNo*pcacheHdr->HOITCACHE_blockSize + NOR_FLASH_START_OFFSET;
        write_nor(  writeAddr, 
                    tempCache->HOITBLK_buf, 
                    pcacheHdr->HOITCACHE_blockSize, 
                    WRITE_KEEP);

        tempCache = tempCache->HOITBLK_cacheListNext;
        writeCount ++;
    }

    return writeCount;
}
/*    
** ��������:    hoitReleaseCache
** ��������:    �ͷ��ڴ��е�cache��
** �䡡��  :    pcacheHdr               cacheͷ�ṹ
** �䡡��  :    д�ص�cache������
** ȫ�ֱ���:
** ����ģ��:    
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
    return LW_TRUE;
}

/*
    �ͷ�cacheͷ
*/
BOOL hoitReleaseCacheHDR(PHOIT_CACHE_HDR pcacheHdr) {
    if (pcacheHdr->HOITCACHE_cacheLineHdr != LW_NULL) {
        __SHEAP_FREE(pcacheHdr->HOITCACHE_cacheLineHdr);
    }
    API_SemaphoreMDelete(&pcacheHdr->HOITCACHE_hLock);
}
/*
    ������һ��Ҫд�Ŀ飬������PHOIT_CACHE_HDR��HOITCACHE_nextBlkToWrite(Ҫд����һ��)
*/
UINT32 hoitFindNextToWrite(PHOIT_CACHE_HDR pcacheHdr, UINT32 cacheType) {
    PHOIT_ERASABLE_SECTOR pSector;
    switch (cacheType)
    {
    case HOIT_CACHE_TYPE_INVALID:
        return (PX_ERROR);
    case HOIT_CACHE_TYPE_DATA:
        if (!pcacheHdr->HOITCACHE_hoitfsVol)
            return (PX_ERROR);
        pSector = pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_now_sector;
        return pSector->HOITS_bno;
    case HOIT_CACHE_TYPE_DATA_EMPTY:
        pSector = pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_erasableSectorList;
        while (pSector != LW_NULL)
        {
            if(pSector->HOITS_uiFreeSize == pcacheHdr->HOITCACHE_blockSize) {
                return pSector->HOITS_bno;
            }
        }
        return (PX_ERROR);
    //TODO ��������gc֮��Ͳ��ܵ����Ľ���һ��д����һ
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
    
}
/*    
** ��������:    hoitFindSector
** ��������:    ����sector�Ż�ȡpSectorָ��
** �䡡��  :    pcacheHdr               cacheͷ�ṹ
                sector_no               sector��
** �䡡��  :    pSectorָ�룬����LW_NULL��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:    
*/
PHOIT_ERASABLE_SECTOR hoitFindSector(PHOIT_CACHE_HDR pcacheHdr, UINT32 sector_no) {
    PHOIT_ERASABLE_SECTOR pSector;
    pSector = pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_erasableSectorList;
    while (pSector != NULL)
    {
        if (pSector->HOITS_bno == sector_no)
            break;
    }
    return pSector;
}

#ifdef HOIT_CACHE_TEST
/*
    cache����
*/
BOOL test_hoit_cache() {
    PHOIT_CACHE_HDR pcacheHdr;
    CHAR            data_write[8] = "1234567\0";
    CHAR            data_read[8];
    INT32           i;
    INT32           j=0;
    INT32           k=0;
    lib_memset(data_read,0,sizeof(data_read));
    printf("======================  hoit cache test   ============================\n");
    pcacheHdr = hoitEnableCache(64, 8, LW_NULL);

    // printf("1.common write read\n");
    // hoitWriteToCache(pcacheHdr, 0, data_write, sizeof(data_write));
    // hoitReadFromCache(pcacheHdr, 4, data_read, sizeof(data_read));
    // printf("result: %s\n",data_read);

    // printf("2.common flush\n");
    // hoitFlushCache(pcacheHdr);
    // lib_memset(data_read,0,sizeof(data_read));
    // read_nor((pcacheHdr->HOITCACHE_nextBlkToWrite-1)*pcacheHdr->HOITCACHE_blockSize+4+NOR_FLASH_START_OFFSET,data_read,sizeof(data_read));
    // printf("result: %s\n",data_read);

    printf("3.mutiple blocks write (no flush)\n");
    for (i=0 ; i < 64 ; i++) {
        k = i/8 + 1;
        j = j%8 + 1;
        data_write[0] = '0' + k;
        data_write[1] = '0' + j;
        hoitWriteThroughCache(pcacheHdr, i*sizeof(data_write), data_write, sizeof(data_write));
    }
    printf("current cache number: %d", pcacheHdr->HOITCACHE_blockNums);
    printf("\nread result:\n");
    for (i=0 ; i<64 ; i++) {
        hoitReadFromCache(pcacheHdr, i*sizeof(data_write), data_read, sizeof(data_read));
        printf("%s\n",data_read);
    }
    printf("\ncache data:\n");

    for (i=0 ; i<64 ; i++) {    /* ��0�鿪ʼ�� */
        read_nor(   0+
                    i*sizeof(data_write)+
                    NOR_FLASH_START_OFFSET,
                    data_read,
                    sizeof(data_read));
        printf("%s\n",data_read);
    }
    printf("\nflushed cache data:\n");
    hoitFlushCache(pcacheHdr);
    for (i=0 ; i<64 ; i++) { /* ��0�鿪ʼ�� */
        read_nor(   0+
                    i*sizeof(data_write)+
                    NOR_FLASH_START_OFFSET,
                    data_read,
                    sizeof(data_read));
        printf("%s\n",data_read);
    }

    printf("\nswap cache block:\n");
    data_write[0] = 'e';
    data_write[1] = 'n';
    data_write[2] = 'd';
    hoitWriteThroughCache(pcacheHdr, 64*sizeof(data_write), data_write, sizeof(data_write));
    hoitReadFromCache(pcacheHdr, 64*sizeof(data_write), data_read, sizeof(data_read));
    printf("%s\n",data_read);
    printf("======================  hoit cache test end  =========================\n");

    return LW_TRUE;
}
#endif
