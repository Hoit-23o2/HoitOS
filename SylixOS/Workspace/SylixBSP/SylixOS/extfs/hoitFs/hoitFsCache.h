/*********************************************************************************************************
**
**                                    �й����??Դ��??
**
**                                   Ƕ��ʽʵʱ����ϵ??
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

#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/include/fs_fs.h"

#include "hoitType.h"
#include "hoitFsLib.h"
#include "SylixOS.h"
#include "../../driver/mtd/nor/nor.h"
/*********************************************************************************************************
 * �ṹ
*********************************************************************************************************/
/* cache Type */
#define HOIT_CACHE_TYPE_INVALID     0
#define HOIT_CACHE_TYPE_DATA        1
#define HOIT_CACHE_TYPE_DATA_EMPTY  2        

/* EBS ���� */
#define HOIT_FILTER_EBS_ENTRY_SIZE                  (sizeof(HOIT_EBS_ENTRY))               /* ��8B */
#define HOIT_FILTER_PAGE_SIZE                       (sizeof(HOIT_EBS_ENTRY) * ( 8 - 1 ))   /* ��56B��7����EBS entry size */
#define HOIT_FILTER_EBS_AREA_SIZE(pcacheHdr)        (pcacheHdr->HOITCACHE_PageAmount * HOIT_FILTER_EBS_ENTRY_SIZE - HOIT_FILTER_PAGE_SIZE)
#define HOIT_FILTER_EBS_MAGIC_NUMBER                0x13579BDF02468ACE                      /* EBS magic number��֮���У���� */
/*********************************************************************************************************
  ��������ƫ���޸ģ�ע������Ĭ��cache���С��flash��sector
*********************************************************************************************************/
/*
    ��������ƫ�ƻ�ȡ��ţ�ƫ�����ַΪNOR_FLASH_START_OFFSET
*/
static inline UINT8 hoitGetSectorNo(UINT32 offset){
    offset += NOR_FLASH_START_OFFSET;
    return GET_SECTOR_NO(offset) - GET_SECTOR_NO(NOR_FLASH_START_OFFSET);
}
/*
    ���������Ż�ȡ����ƫ�ƣ�ƫ�����ַΪNOR_FLASH_START_OFFSET
*/
static inline UINT32 hoitGetSectorOffset(UINT8 sector_no){
    sector_no += GET_SECTOR_NO(NOR_FLASH_START_OFFSET);
    return _G_am29LV160DB_sector_infos[sector_no].sector_start_offset - NOR_FLASH_START_OFFSET;
}
/*
    ��ȡһ��flash sector ԭʼ��С��64KB��
*/
static inline UINT hoitGetSectorSize(UINT8 sector_no){
    sector_no += GET_SECTOR_NO(NOR_FLASH_START_OFFSET);

    if(sector_no < 0 || sector_no >= NOR_FLASH_NSECTOR){
        return -1;
    }
    UINT origin_sector_size = 1024 * (_G_am29LV160DB_sector_infos[sector_no].sector_size);
//    UINT pageNum = origin_sector_size/(HOIT_FILTER_EBS_ENTRY_SIZE + HOIT_FILTER_PAGE_SIZE);
    return origin_sector_size;
}
/*
    ��ȡflash sector��regoin_no
*/
static inline UINT8 hoitGetSectorRegion(UINT8 sector_no){
    sector_no += GET_SECTOR_NO(NOR_FLASH_START_OFFSET);
    if(sector_no < 0 || sector_no >= NOR_FLASH_NSECTOR){
        return -1;
    }
    return _G_am29LV160DB_sector_infos[sector_no].region_no;     
}
/*
    ���flash sector�Ƿ����
*/
static inline BOOL hoiCheckSectorDirty(UINT32 base, UINT8 sector_no){
    INT i;
    GET_SECTOR_NO(NOR_FLASH_START_OFFSET);
    if(sector_no < 0 || sector_no >= NOR_FLASH_NSECTOR){
        return TRUE;
    }
    UINT sector_size = GET_SECTOR_SIZE(sector_no);
    UINT32 addr = GET_SECTOR_OFFSET(sector_no) + base;
    volatile UINT8* p;
    for (i = 0; i < sector_size; i++)
    {
        p = (volatile UINT8*)(addr + i);
        if((*p & 0xFF) != 0xFF){
          return TRUE;
        }
    }
    return FALSE;
}





/*********************************************************************************************************
 * cache�㺯��
*********************************************************************************************************/
PHOIT_CACHE_HDR     hoitEnableCache(UINT32 uiCacheBlockSize, 
                                    UINT32 uiCacheBlockNums, 
                                    PHOIT_VOLUME phoitfs);
BOOL                hoitFreeCache(PHOIT_CACHE_HDR pcacheHdr);                                    
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
UINT32              hoitFlushCache(PHOIT_CACHE_HDR pcacheHdr, 
                                    PHOIT_CACHE_BLK pcache);
UINT32              hoitFindNextToWrite(PHOIT_CACHE_HDR pcacheHdr, 
                                        UINT32 cacheType,
                                        UINT32 uiSize);
BOOL                hoitWriteThroughCache(PHOIT_CACHE_HDR pcacheHdr, 
                                          UINT32 uiOfs, 
                                          PCHAR pContent, 
                                          UINT32 uiSize);
UINT32              hoitWriteToCache(PHOIT_CACHE_HDR pcacheHdr,
                                     PCHAR pContent, 
                                     UINT32 uiSize);
PHOIT_ERASABLE_SECTOR hoitFindSector(PHOIT_CACHE_HDR pcacheHdr, 
                                     UINT32 sector_no);
VOID                hoitResetSectorState(PHOIT_CACHE_HDR pcacheHdr, 
                                         PHOIT_ERASABLE_SECTOR pErasableSector); 
VOID                hoitOccupySectorState(PHOIT_CACHE_HDR pcacheHdr, 
                                          PHOIT_ERASABLE_SECTOR pErasableSector);   
VOID                hoitWriteBackCache(PHOIT_CACHE_HDR pcacheHdr, 
                                        PHOIT_CACHE_BLK pcache);

/*********************************************************************************************************
 * filter�㺯��
*********************************************************************************************************/
UINT32              hoitInitEBS(PHOIT_CACHE_HDR pcacheHdr, 
                                    UINT32 uiCacheBlockSize);
UINT32              hoitUpdateEBS(PHOIT_CACHE_HDR pcacheHdr, 
                                    PHOIT_CACHE_BLK pcache, 
                                    UINT32 inode, 
                                    UINT32 offset);
VOID                __hoit_mark_obsolete(PHOIT_VOLUME pfs, 
                                        PHOIT_RAW_HEADER pRawHeader, 
                                        PHOIT_RAW_INFO pRawInfo);
VOID                hoitCheckEBS(PHOIT_VOLUME pfs, 
                                    UINT32 sector_no, 
                                    UINT32 n); 
inline UINT32       hoitEBSupdateCRC(PHOIT_CACHE_HDR pcacheHdr, 
                                    PHOIT_CACHE_BLK pcache, 
                                    UINT32 sector_no);
UINT32              hoitEBSEntryAmount(PHOIT_VOLUME pfs, 
                                        UINT32 sector_no); 
UINT32              hoitSectorGetNextAddr(PHOIT_CACHE_HDR pcacheHdr, 
                                            UINT32 sector_no, 
                                            UINT i, 
                                            UINT32 *obsoleteFlag);
BOOL                hoitCheckSectorCRC(PHOIT_CACHE_HDR pcacheHdr, 
                                            UINT32 sector_no); 
//! ZN ��ʱ�ò���
// inline UINT32       hoitPhysToFlash(PHOIT_CACHE_HDR pcacheHdr, 
//                                             UINT32 phys_addr);                                                                                                       
#ifdef HOIT_CACHE_TEST
BOOL    test_hoit_cache();
#endif

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSCACHE_H_ */
