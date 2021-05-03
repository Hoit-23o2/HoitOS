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
** ��   ��   ��: hoitFsLog.c
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 04 �� 25 ��
**
** ��        ��: ��־��ʵ��
*********************************************************************************************************/
#include "hoitFsLog.h"
#include "hoitFsLib.h"
#include "hoitFsCache.h"

VOID __hoitInserteLogSector(PHOIT_VOLUME pfs, PHOIT_LOG_SECTOR pLogSectorNew){
    PHOIT_LOG_SECTOR    pLogSectorTraverse;
    PHOIT_LOG_SECTOR    pLogSectorTrailling;


    pLogSectorTraverse = pfs->HOITFS_logInfo->pLogSectorList;

    while (pLogSectorTraverse)
    {
        pLogSectorTrailling = pLogSectorTraverse;
        pLogSectorTraverse = pLogSectorTraverse->pErasableNextLogSector;
    }

    // PHOIT_FRAG_TREE_LIST_NODE pFTlistTrailling;
    // PHOIT_FRAG_TREE_LIST_NODE pFTlistTraverse = pFTlistHeader->pFTlistHeader;
    // while (pFTlistTraverse != LW_NULL)
    // {
    //     pFTlistTrailling = pFTlistTraverse;
    //     pFTlistTraverse = pFTlistTraverse->pFTlistNext;
    // }
    // pFTlistTrailling->pFTlistNext = pTFlistNode;
    
}

VOID __hoitSearchLogSector(PHOIT_VOLUME pfs,PHOIT_LOG_SECTOR pLogSector){
    
}


PHOIT_LOG_INFO hoitLogInit(PHOIT_VOLUME pfs, UINT uiLogSize, UINT uiSectorNum){
    PHOIT_RAW_LOG           pRawLog;
    PHOIT_LOG_INFO          pLogInfo;
    
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    PHOIT_LOG_SECTOR        pLogSector;

    if(uiLogSize == 0 || uiSectorNum == 0){
        return LW_NULL;
    }
    
    pRawLog = (PHOIT_RAW_LOG)lib_malloc(sizeof(HOIT_RAW_LOG));
    lib_memset(pRawLog, 0, sizeof(HOIT_RAW_LOG));
    pRawLog->file_type = S_IFLOG;
    pRawLog->magic_num = HOIT_MAGIC_NUM;
    pRawLog->flag      = HOIT_FLAG_TYPE_LOG | HOIT_FLAG_OBSOLETE;
    pRawLog->ino       = __hoit_alloc_ino(pfs);
    pRawLog->totlen    = sizeof(HOIT_RAW_LOG);
    pRawLog->version   = pfs->HOITFS_highest_version++;
    //TODO: �ҵ�һ���յ�Sector��ΪLOG Sector
    //!��Build��ʱ�����û��ɨ�赽LOG���͵���InitLOG��
    //!�������OPEN LOG
    pRawLog->uiLogFirstAddr = pErasableSector->HOITS_addr;
    pRawLog->uiLogSize = uiLogSize;
    hoitWriteToCache(pfs->HOITFS_cacheHdr, (PCHAR)pRawLog, pRawLog->totlen);

    pLogSector = (PHOIT_LOG_SECTOR)lib_malloc(sizeof(HOIT_LOG_SECTOR));
    lib_memset(pLogSector, 0, sizeof(HOIT_LOG_SECTOR));
    lib_memcpy(&pLogSector->ErasableSetcor,  pErasableSector, sizeof(HOIT_ERASABLE_SECTOR));
    
    pLogInfo = (PHOIT_LOG_INFO)lib_malloc(sizeof(HOIT_LOG_INFO));
    lib_memset(pLogInfo, 0, sizeof(HOIT_LOG_INFO));
    pLogInfo->pLogSectorList = pLogSector;
    
    pfs->HOITFS_logInfo          = pLogInfo;

    return pLogInfo;
}

PHOIT_LOG_INFO hoitLogOpen(PHOIT_VOLUME pfs, PHOIT_RAW_LOG pRawLog){
    PHOIT_LOG_INFO          pLogInfo;
    PHOIT_ERASABLE_SECTOR   pErasableLogSector;
    PHOIT_ERASABLE_SECTOR   pErasableSectorTraverse;

    pLogInfo = (PHOIT_LOG_INFO)lib_malloc(sizeof(HOIT_LOG_INFO));
    lib_memset(pLogInfo, 0, sizeof(HOIT_LOG_INFO));
    
    pErasableLogSector = LW_NULL;
    pErasableSectorTraverse = pfs->HOITFS_erasableSectorList;
    while (pErasableSectorTraverse)
    {
        if(pErasableSectorTraverse->HOITS_addr == pRawLog->uiLogFirstAddr){
            pErasableLogSector = pErasableSectorTraverse;
            break;
        }
    }
    if(pErasableLogSector == LW_NULL){
#ifdef LOG_DEBUG
        printf("[%s] not found log setcor\n", __func__);
#endif // LOG_DEBUG
        return LW_NULL;
    }

    
}

VOID hoitLogRead(PHOIT_VOLUME pfs, PHOIT_LOG_INFO pLogInfo, UINT uiOfs, PCHAR pLog, UINT uiSize){

}

VOID hoitLogAppend(PHOIT_VOLUME pfs, PCHAR pLog){

}
