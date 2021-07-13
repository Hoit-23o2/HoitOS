/*********************************************************************************************************
**
**                                    �й������Դ��֯
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
#include "hoitFsGC.h"
#include "../../driver/mtd/nor/nor.h"

//TODO: 1. now_sector������LOG�ص�������Ϊɶ�����ص��ˣ�    DONE
//TODO: 2. ǿ��GC                                         DONE  
//TODO: 3. ע���޿ռ��˾ͱ���                              DONE 
//TODO: 4. 
/*********************************************************************************************************
 * �궨��
*********************************************************************************************************/
#define __HOITFS_CACHE_LOCK(pcacheHdr)          API_SemaphoreMPend(pcacheHdr->HOITCACHE_hLock, \
                                                LW_OPTION_WAIT_INFINITE)
#define __HOITFS_CACHE_UNLOCK(pcacheHdr)        API_SemaphoreMPost(pcacheHdr->HOITCACHE_hLock)

/*********************************************************************************************************
                                           API ����
*********************************************************************************************************/
/*********************************************************************************************************
 * �����������룬���ڻ���flash���ݵ��ڴ�
*********************************************************************************************************/

/*    
** ��������:    hoitEnableCache
** ��������:    ��ʼ�� hoitfs cache
** �䡡��  :    uiCacheBlockSize       ����cache��С
**              uiCacheBlockNums       cache�������
**              phoitfs                hoitfs�ļ���ṹ��
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

    hoitInitFilter(pcacheHdr,uiCacheBlockSize);

    return pcacheHdr;
}
/*    
** ��������:    hoitFreeCache
** ��������:    ��ʼ�� hoit cache
** �䡡��  :    uiCacheBlockSize       ����cache��С
**              uiCacheBlockNums       cache�������
**              phoitfs                hoitfs�ļ���ṹ��
** �䡡��  : LW_NULL ��ʾʧ�ܣ�PHOIT_CACHE_HDR��ַ ��ʾ�ɹ�
** ȫ�ֱ���:
** ����ģ��:    
*/
BOOL hoitFreeCache(PHOIT_CACHE_HDR pcacheHdr) {
    PHOIT_CACHE_BLK pcache;
    PHOIT_CACHE_BLK pfree;
    if (pcacheHdr == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "hoitfs cache header is null.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);        
    }   

    pcache = pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext;
    while (pcache != pcacheHdr->HOITCACHE_cacheLineHdr)
    {
        pcache->HOITBLK_cacheListPrev->HOITBLK_cacheListNext = pcache->HOITBLK_cacheListNext;   /* �Ͽ�pcache */
        pcache->HOITBLK_cacheListNext->HOITBLK_cacheListPrev = pcache->HOITBLK_cacheListPrev;
        pfree = pcache;
        pcache = pcache->HOITBLK_cacheListNext;
        __SHEAP_FREE(pfree->HOITBLK_buf);
        __SHEAP_FREE(pfree);        
    }
    __SHEAP_FREE(pcache->HOITBLK_buf);  /* �ͷ�ѭ������ͷ */
    __SHEAP_FREE(pcacheHdr);
    if(pcacheHdr->HOITCACHE_hLock){
        API_SemaphoreMDelete(&pcacheHdr->HOITCACHE_hLock);
    }
    return ERROR_NONE;
}

/*    
** ��������:    hoitAllocCache
** ��������:    ����cache,��ȡӳ����flash������
** �䡡��  :    pcacheHdr               cacheͷ�ṹ
**              flashBlkNo              ����cacheӳ���������
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

    if (!pcacheHdr || !pSector) {
        return LW_NULL;
    }
    
    if (pcacheHdr->HOITCACHE_blockNums == pcacheHdr->HOITCACHE_blockMaxNums) { 
        /* cache����������������Ҫ���� */
        flag = LW_TRUE;
        //TOOPT �����㷨��ʱ����FIFO�����滻����ͷ����һ���ڵ㣨����β��
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
            return LW_NULL;
        }

        pcache->HOITBLK_buf = (PCHAR)__SHEAP_ALLOC(cacheBlkSize);
        if (pcache->HOITBLK_buf == NULL) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
            return LW_NULL;
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
**              flashBlkNo              ������
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
** ��������:    ��һ�θ������ȵ�����ʵ��д���ض���ַ��
** �䡡��  :    pcacheHdr               cacheͷ�ṹ
**              uiOfs                   ����д����ʼλ�ã��ϲ㽫flash�����е�NOR_FLASH_START_OFFSETλ����Ϊ0��ַ��
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
    UINT32  i;                      /* ��һ��Ҫд���flash�� */
    UINT32                  writeAddr = uiOfs + writeBytes + NOR_FLASH_START_OFFSET;
    PHOIT_CACHE_BLK         pcache;
    PHOIT_ERASABLE_SECTOR   pSector = pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_erasableSectorList;
    
    //! 22021-07-07 ��Ȼ���д������Ҫ���ڱ�ǹ��ڣ��ĳ�д�������Լ�FIFO�ȽϺ�
    while(uiSize != 0) {
        size_t  stBufSize = (cacheBlkSize - stStart);
        i = (uiOfs + writeBytes)/cacheBlkSize;

        pSector = hoitFindSector(pcacheHdr, i);/* ���ҿ�Ŷ�Ӧ��pSector */

        writeAddr = uiOfs + writeBytes + NOR_FLASH_START_OFFSET;

        pcache = hoitCheckCacheHit(pcacheHdr, i);
        if (stBufSize > uiSize) { /* ����д������sector */
            if (pcache == LW_NULL) { /* δ���� */
                // pcache = hoitAllocCache(pcacheHdr, i, HOIT_CACHE_TYPE_DATA, pSector);
                // if (pcache == LW_NULL) { /* δ�ɹ�����cache��ֱ��д��flash */                      
                //     write_nor(writeAddr, pContent, uiSize, WRITE_KEEP);
                // }
                // else { /* �ɹ�����cache����д��cache */
                //     lib_memcpy(pcache->HOITBLK_buf + stStart, pucDest, uiSize);
                // }
                write_nor(writeAddr, pContent, uiSize, WRITE_KEEP);
            } else {    /* cache������ֱ��д�����ݣ�����������ǰ������ͷ */
                lib_memcpy(pcache->HOITBLK_buf + stStart, pucDest, uiSize);
                //! 2021-07-04 ZN cache�滻�޸�ΪLRU
                // pcache->HOITBLK_cacheListPrev->HOITBLK_cacheListNext = pcache->HOITBLK_cacheListNext;
                // pcache->HOITBLK_cacheListNext->HOITBLK_cacheListPrev = pcache->HOITBLK_cacheListPrev;
                
                // pcache->HOITBLK_cacheListPrev = pcacheHdr->HOITCACHE_cacheLineHdr;
                // pcache->HOITBLK_cacheListNext = pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext;
                
                // pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext->HOITBLK_cacheListPrev = pcache;
                // pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext = pcache;                
            }

            writeBytes += uiSize;
            if (stStart + uiSize > pSector->HOITS_offset) {
                /* ���write throughλ�ô��ڵ�ǰsectorƫ�ƣ�����Ҫ�޸� */
                pSector->HOITS_offset       = stStart + uiSize;
                pSector->HOITS_uiUsedSize   = stStart + uiSize;
                pSector->HOITS_uiFreeSize   = cacheBlkSize - (stStart + uiSize);
                pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_totalUsedSize += uiSize;
            }
            uiSize      = 0;
            break;
        } else { /* д������sector */
            if (pcache == LW_NULL) { /* δ���� */
                // pcache = hoitAllocCache(pcacheHdr, i, HOIT_CACHE_TYPE_DATA, pSector);
                // if (pcache == LW_NULL) { /* δ�ɹ�����cache��ֱ��д��flash */
                //     write_nor(writeAddr, pContent, stBufSize, WRITE_KEEP);
                // }
                // else { /* �ɹ�����cache����д��cache */
                //     lib_memcpy(pcache->HOITBLK_buf + stStart, pucDest, stBufSize);
                // }
                write_nor(writeAddr, pContent, uiSize, WRITE_KEEP);
            } else {
                lib_memcpy(pcache->HOITBLK_buf + stStart, pucDest, stBufSize);
                //! 2021-07-04 ZN cache�滻�޸�ΪLRU
                // pcache->HOITBLK_cacheListPrev->HOITBLK_cacheListNext = pcache->HOITBLK_cacheListNext;
                // pcache->HOITBLK_cacheListNext->HOITBLK_cacheListPrev = pcache->HOITBLK_cacheListPrev;
                
                // pcache->HOITBLK_cacheListPrev = pcacheHdr->HOITCACHE_cacheLineHdr;
                // pcache->HOITBLK_cacheListNext = pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext;
                
                // pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext->HOITBLK_cacheListPrev = pcache;
                // pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext = pcache;                 
            }

            pucDest     += stBufSize;
            uiSize      -= stBufSize;
            writeBytes  += stBufSize;
            stStart      = 0;

            pSector->HOITS_offset       = cacheBlkSize;
            pSector->HOITS_uiFreeSize   = 0;
            pSector->HOITS_uiUsedSize   = cacheBlkSize;
        }
    }
    // if(uiSize >= sizeof(HOIT_RAW_HEADER)){  /* ʵ��ͷ */
    //     PHOIT_RAW_HEADER headr = (PHOIT_RAW_HEADER)pContent;
    //     headr->magic_num == HOIT_MAGIC_NUM
    //     {
    //         pSector.usedCount++;               
    //     }
    // }
    return LW_TRUE;   
}
/*    
** ��������:    hoitUpdateEBS
** ��������:    ��д��һ������ʵ��֮�󣬵��øú���������Ӧ��EBS entry 
** �䡡��  :    pcache                   cache��ṹ��
                inode                   ����ʵ���Ӧ���ļ�inode��
                pageNum                 ����ʵ����ռҳ��
                offset                  ����ʵ��д���sector��λ��
** �䡡��  :    
** ȫ�ֱ���:
** ����ģ��:    
*/
UINT32 hoitUpdateEBS(PHOIT_CACHE_HDR pcacheHdr, PHOIT_CACHE_BLK pcache, UINT32 inode, UINT32 pageNum, UINT32 offset) {
    UINT32          startPageNo = offset/HOIT_FILTER_PAGE_SIZE;     /* ��ʼҳ�� */
    UINT32          i;
    PHOIT_EBS_ENTRY pentry;
    if(pcacheHdr==LW_NULL || pcache== LW_NULL) {
        return PX_ERROR;
    }

    pentry      = (PHOIT_EBS_ENTRY)(pcache->HOITBLK_buf + pcacheHdr->HOITCACHE_EBSStartAddr + (size_t)startPageNo * HOIT_FILTER_EBS_ENTRY_SIZE);

    // if ( (pentry+pageNum-1) > pcacheHdr->HOITCACHE_blockSize ) {    /* Խ���� */
    //     return PX_ERROR;
    // }

    for(i=0 ; i<pageNum ; i++) {        /* �������EBS entry */
        pentry->HOIT_EBS_ENTRY_inodeNo  = inode;
        pentry->HOIT_EBS_ENTRY_obsolete = UINT_MAX;
        pentry ++;
    }
    return ERROR_NONE;
}
/*    
** ��������:    hoitWriteToCache
** ��������:    ��һ�θ������ȵ�����ʵ��д��cache��cache�Զ����ҿ���װ�¸�����ʵ���λ�ý���д��
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
    UINT32  inode;          /* ����ʵ�������ļ�inode�� */
    UINT32  pageNum;        /* ����ʵ����Ҫҳ���� */
    PHOIT_CACHE_BLK         pcache;
    PHOIT_ERASABLE_SECTOR   pSector = pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_erasableSectorList;

    //! ���ͷ�����
    if(uiSize < sizeof(HOIT_RAW_HEADER)){
        /* ��������ʵ�� */
        return PX_ERROR;
    }
    if(((PHOIT_RAW_HEADER)pContent)->magic_num != HOIT_MAGIC_NUM){
        /* ��������ʵ�� */
        return PX_ERROR;
    }

    if (pSector == LW_NULL) {
        return PX_ERROR;
    }
    
    if ((size_t)uiSize > pcacheHdr->HOITCACHE_blockSize ) {
        return PX_ERROR;
    }

    //! 2021-07-04 ���EBS����
    inode       = ((PHOIT_RAW_HEADER)pContent)->ino;
    pageNum     = uiSize%HOIT_FILTER_PAGE_SIZE?uiSize/HOIT_FILTER_PAGE_SIZE:uiSize/HOIT_FILTER_PAGE_SIZE+1;

    //i = hoitFindNextToWrite(pcacheHdr, HOIT_CACHE_TYPE_DATA, uiSize);
    //! ���ҳ����
    i = hoitFindNextToWrite(pcacheHdr, HOIT_CACHE_TYPE_DATA, pageNum*HOIT_FILTER_PAGE_SIZE);

    if (i == PX_ERROR) {
        return PX_ERROR;
    } else {
        pSector = hoitFindSector(pcacheHdr, i);      
    }

    writeAddr = pSector->HOITS_bno * 
                pcacheHdr->HOITCACHE_blockSize + 
                pSector->HOITS_offset + 
                NOR_FLASH_START_OFFSET;         /* ȷ������ʵ��д���flash��ַ */

    pcache = hoitCheckCacheHit(pcacheHdr, pSector->HOITS_bno);
    if (pcache == LW_NULL) {                                    /* δ���� */
        pcache = hoitAllocCache(pcacheHdr, pSector->HOITS_bno, HOIT_CACHE_TYPE_DATA, pSector);
        if (pcache == LW_NULL) {                                /* δ�ɹ�����cache��ֱ��д��flash */                        
            write_nor(writeAddr, pContent, uiSize, WRITE_KEEP);
        }
        else {                                                  /* �ɹ�����cache����д��cache��������cache���ᵽ��ͷ */
            lib_memcpy(pcache->HOITBLK_buf + pSector->HOITS_offset, pucDest, uiSize);
        }
    } else {    /* cache������ֱ��д�����ݣ�����������ǰ������ͷ */
        lib_memcpy(pcache->HOITBLK_buf + pSector->HOITS_offset, pucDest, uiSize);
        //! 2021-07-04 ZN cache�滻�޸�ΪLRU
        pcache->HOITBLK_cacheListPrev->HOITBLK_cacheListNext = pcache->HOITBLK_cacheListNext;
        pcache->HOITBLK_cacheListNext->HOITBLK_cacheListPrev = pcache->HOITBLK_cacheListPrev;
        
        pcache->HOITBLK_cacheListPrev = pcacheHdr->HOITCACHE_cacheLineHdr;
        pcache->HOITBLK_cacheListNext = pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext;
        
        pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext->HOITBLK_cacheListPrev = pcache;
        pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext = pcache;

    }

    /* ����EBS entry */
    hoitUpdateEBS(pcacheHdr, pcache, inode, pageNum, pSector->HOITS_offset);

    /* ����HOITFS_now_sector */
    pSector->HOITS_offset       += uiSize;
    pSector->HOITS_uiFreeSize   -= uiSize;
    pSector->HOITS_uiUsedSize   += uiSize;
    pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_totalUsedSize += uiSize;

    /* ��ǰд�Ŀ����ˣ���ȥ����һ�����п��еĿ� */
    //! ��ȥEBS����
    if (pSector->HOITS_uiFreeSize  == 0) {
        pSector = pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_erasableSectorList;
        i = hoitFindNextToWrite(pcacheHdr, HOIT_CACHE_TYPE_DATA, sizeof(HOIT_RAW_HEADER));
        if (i != PX_ERROR) {
            pSector = hoitFindSector(pcacheHdr, i);
        }
    }

    pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_now_sector = pSector;

    
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
    UINT32  writeAddr;
    if (pcacheHdr->HOITCACHE_blockNums == 0)
        return 0;
    
    tempCache = pcacheHdr->HOITCACHE_cacheLineHdr->HOITBLK_cacheListNext;
    while (tempCache != pcacheHdr->HOITCACHE_cacheLineHdr) {
        if (tempCache->HOITBLK_sector == LW_NULL)
            continue;
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
    pcacheHdr   cacheͷ
    cacheType   ������
    uiSize      ���ʣ��ռ�Ҫ��ֻ��cacheType == HOIT_CACHE_TYPE_DATA�²������塣
                ���HOITFS_now_sector�ռ���㣬��Ĭ�Ϸ���HOITFS_now_sector��
*/
UINT32 hoitFindNextToWrite(PHOIT_CACHE_HDR pcacheHdr, UINT32 cacheType, UINT32 uiSize) {
    //TOOPT: 2021-07-04 �����ǰ��д���£��Ƿ�����ȴ�cache�����ܷŵ�������ʵ��Ŀ��п飬��ȥ����sector�б����ң�
    //! 2021-07-04 ZN ���EBS������uiSize�Ƚ�ʱ����һ��cache���д�ռ䡣
    PHOIT_ERASABLE_SECTOR pSector;
    switch (cacheType)
    {
    case HOIT_CACHE_TYPE_INVALID:
        return (PX_ERROR);
    case HOIT_CACHE_TYPE_DATA:
        pSector = pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_now_sector;
        /* �����ǰ��д���£�����һ�� */
        if (pSector->HOITS_uiFreeSize  < (size_t)uiSize) {
            pSector = pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_erasableSectorList;
        } else {
            return pSector->HOITS_bno;
        }

        while (pSector != LW_NULL) {
            if(!hoitLogCheckIfLog(pcacheHdr->HOITCACHE_hoitfsVol, pSector)                  /* ������LOG SECTOR*/
               && pSector != pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_now_sector){            /* �Ҳ���NOW SECTORʱ���ż�� */
                if(pSector->HOITS_uiFreeSize  >= (size_t)uiSize) {
                    return pSector->HOITS_bno;
                }
            }
            pSector = pSector->HOITS_next;
        }      
        if (pSector == LW_NULL) {                                   /* flash�ռ����岻�㣬��ʼִ��ǿ��GC */
            hoitGCForegroundForce(pcacheHdr->HOITCACHE_hoitfsVol);
        }

        /* GC֮�������ҿ� */
        pSector = pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_erasableSectorList;

        while (pSector != LW_NULL) {
            if(!hoitLogCheckIfLog(pcacheHdr->HOITCACHE_hoitfsVol, pSector)                  /* ������LOG SECTOR*/
               && pSector != pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_now_sector){            /* �Ҳ���NOW SECTORʱ���ż�� */
                if(pSector->HOITS_uiFreeSize  >= (size_t)uiSize) {
                    return pSector->HOITS_bno;
                }
            }
            pSector = pSector->HOITS_next;
        }
        /* ��Ȼû�п��ÿ��򷵻ش��� */
        return PX_ERROR;

    case HOIT_CACHE_TYPE_DATA_EMPTY:
        pSector = pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_erasableSectorList;
        while (pSector != LW_NULL)
        {
            if(!hoitLogCheckIfLog(pcacheHdr->HOITCACHE_hoitfsVol, pSector)                  /* ������LOG SECTOR*/
               && pSector != pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_now_sector){            /* �Ҳ���NOW SECTORʱ���ż�� */
                if(pSector->HOITS_uiFreeSize == pcacheHdr->HOITCACHE_blockSize) {
                    return pSector->HOITS_bno;
                }
            }
            pSector = pSector->HOITS_next;
        }
        /* �Ҳ���������GC */
        if (pSector == LW_NULL) {
            hoitGCForegroundForce(pcacheHdr->HOITCACHE_hoitfsVol);
            while (pSector != LW_NULL)
            {
                //! 2021-05-04 Modified By PYQ
                if(!hoitLogCheckIfLog(pcacheHdr->HOITCACHE_hoitfsVol, pSector)                  /* ������LOG SECTOR*/
                && pSector != pcacheHdr->HOITCACHE_hoitfsVol->HOITFS_now_sector){            /* �Ҳ���NOW SECTORʱ���ż�� */
                    if(pSector->HOITS_uiFreeSize == pcacheHdr->HOITCACHE_blockSize) {
                        return pSector->HOITS_bno;
                    }
                }
                pSector = pSector->HOITS_next;
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
** ��������: hoitResetSectorState
** ��������: ����һ��Sector��״̬�� Added By PYQ
** �䡡��  : pcacheHdr        ������Ϣͷ��
**          pErasableSector   Ŀ��Sector
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*/
VOID hoitResetSectorState(PHOIT_CACHE_HDR pcacheHdr, PHOIT_ERASABLE_SECTOR pErasableSector){
    pErasableSector->HOITS_uiFreeSize = pErasableSector->HOITS_length;
    pErasableSector->HOITS_uiUsedSize = 0;
    pErasableSector->HOITS_offset     = 0;
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
    while (pSector != LW_NULL)
    {
        if (pSector->HOITS_bno == sector_no)
            break;
        pSector = pSector->HOITS_next;
    }
    return pSector;
}
//! 2021-07-04 ZN �������˲�
/*********************************************************************************************************
 * ���˲�������룬flash�е�EBS������ϲ�͸��������һ���д�����ݽ���ת����
 * !ע�����
 *      1. ÿ��sector�� free size ��ʱ�����Ҫ��ȥһ��EBS�����С������ϲ���δ֪��
*********************************************************************************************************/

/*********************************************************************************************************
 * EBS���
 * EBS����ʽ�ӣ�uiCacheBlockSize = (PageSize + EBSEntrySize) * PageAmount
 *  flash head                       Nor Flash                  flash end
 * --------------------------------------------------------------------------
 *                                                         |
 *                   data entry area                       |  EBS entry area
 *                                                         |
 * --------------------------------------------------------------------------
 * ������£�
 *          EBSEntrySize    = 8B
 *              EBSEntryǰ32λ�����ҳ�����ļ���inode�ţ�ȫ1��ʾ��ҳδд������
 *              ��32λΪ���ڱ����inode�ţ�ǰ32λ����Ϊȫ1ʱ�������壬ȫ1ʱ��ʾδ���ڣ�ȫ0Ϊ���ڡ�
 *          PageSize        = 56B
 *          PageAmount      =  64KB / 64B = 1K    
*********************************************************************************************************/ 
/*    
** ��������:    hoitInitFilter
** ��������:    ��ʼ�����˲㣬����EBS����
** �䡡��  :    pcacheHdr               cacheͷ�ṹ
                uiCacheBlockSize        ����cache��С����ʱΪһ��sector��С��64KB(�豸ΪAm29LV160DB)
** �䡡��  :    pSectorָ�룬����LW_NULL��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:    
*/
UINT32 hoitInitFilter(PHOIT_CACHE_HDR pcacheHdr, UINT32 uiCacheBlockSize) {
    if (!pcacheHdr || uiCacheBlockSize < (HOIT_FILTER_EBS_ENTRY_SIZE+HOIT_FILTER_PAGE_SIZE)) {
        return PX_ERROR;
    }
    pcacheHdr->HOITCACHE_PageAmount     = uiCacheBlockSize/(HOIT_FILTER_EBS_ENTRY_SIZE+HOIT_FILTER_PAGE_SIZE);
    pcacheHdr->HOITCACHE_EBSStartAddr   = HOIT_FILTER_PAGE_SIZE * pcacheHdr->HOITCACHE_PageAmount;
    return ERROR_NONE;
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
