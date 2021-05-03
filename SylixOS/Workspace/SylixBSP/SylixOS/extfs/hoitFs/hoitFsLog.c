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

PHOIT_LOG_INFO hoitLogInit(PHOIT_VOLUME pfs, UINT uiLogSize, UINT uiSectorNum){
    PHOIT_RAW_LOG           pRawLog;
    PHOIT_LOG_INFO          pLogInfo;
    
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    PHOIT_LOG_SECTOR        pLogSector;

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
    pRawLog->uiLogAddr = pErasableSector->HOITS_addr;
    pRawLog->uiLogSize = uiLogSize;
    hoitWriteToCache(pfs->HOITFS_cacheHdr, (PCHAR)pRawLog, pRawLog->totlen);

    pLogSector = (PHOIT_LOG_SECTOR)lib_malloc(sizeof(HOIT_LOG_SECTOR));
    lib_memset(pLogSector, 0, sizeof(HOIT_LOG_SECTOR));
    lib_memcpy(&pLogSector->ErasableSetcor,  pErasableSector, sizeof(HOIT_ERASABLE_SECTOR));

    pLogInfo = (PHOIT_LOG_INFO)lib_malloc(sizeof(HOIT_LOG_INFO));
    lib_memset(pLogInfo, 0, sizeof(HOIT_LOG_INFO));
    
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
        if(pErasableSectorTraverse->HOITS_addr == pRawLog->uiLogAddr){
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

    pLogInfo->pLogSector = pErasableLogSector;
}

VOID hoitLogRead(PHOIT_VOLUME pfs, PHOIT_LOG_INFO pLogInfo, UINT uiOfs, PCHAR pLog, UINT uiSize){

}

VOID hoitLogAppend(PHOIT_VOLUME pfs, PCHAR pLog){

}
