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
** 文   件   名: hoitFsLog.c
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 04 月 25 日
**
** 描        述: 日志层实现
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
    pLogSectorTrailling->pErasableNextLogSector = pLogSectorNew;
}

PHOIT_ERASABLE_SECTOR __hoitFindAvailableSector(PHOIT_VOLUME pfs){
    UINT                    uiAvaiSectorNum;
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    
    uiAvaiSectorNum = hoitFindNextToWrite(pfs->HOITFS_cacheHdr, );
    if(uiAvaiSectorNum == PX_ERROR){
        return LW_NULL;
    }
    
    return pErasableSector;
}

PHOIT_LOG_INFO hoitLogInit(PHOIT_VOLUME pfs, UINT uiLogSize, UINT uiSectorNum){
    //!在Build的时候，如果没有扫描到LOG，就调用InitLOG，
    //!否则调用OPEN LOG

    PHOIT_RAW_LOG           pRawLog;
    PHOIT_LOG_INFO          pLogInfo;
    
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    PHOIT_LOG_SECTOR        pLogSector;

    if(uiLogSize == 0 || uiSectorNum == 0){
        return LW_NULL;
    }

    if(uiSectorNum > 1){
#ifdef LOG_DEBUG
        printf("[%s] only support one log sector for now \n", __func__);
#endif // LOG_DEBUG
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
    
    //TODO: 找到一个空的Sector作为LOG Sector
    pErasableSector = __hoitFindAvailableSector(pfs);

    if(pErasableSector == LW_NULL){
#ifdef LOG_DEBUG
        printf("[%s] can't find a sector for log\n", __func__);
#endif // LOG_DEBUG
        pfs->HOITFS_logInfo = LW_NULL;
        return LW_NULL;
    }
    pRawLog->uiLogFirstAddr = pErasableSector->HOITS_addr;
    pRawLog->uiLogSize = uiLogSize;
    hoitWriteToCache(pfs->HOITFS_cacheHdr, (PCHAR)pRawLog, pRawLog->totlen);



    pLogInfo = (PHOIT_LOG_INFO)lib_malloc(sizeof(HOIT_LOG_INFO));
    lib_memset(pLogInfo, 0, sizeof(HOIT_LOG_INFO));
    
    pLogSector = (PHOIT_LOG_SECTOR)lib_malloc(sizeof(HOIT_LOG_SECTOR));
    lib_memset(pLogSector, 0, sizeof(HOIT_LOG_SECTOR));
    lib_memcpy(&pLogSector->ErasableSetcor,  pErasableSector, sizeof(HOIT_ERASABLE_SECTOR));
    
    pLogInfo->pLogSectorList           = pLogSector;
    pLogSector->pErasableNextLogSector = LW_NULL;
    
    pfs->HOITFS_logInfo      = pLogInfo;

    return pLogInfo;
}

PHOIT_LOG_INFO hoitLogOpen(PHOIT_VOLUME pfs, PHOIT_RAW_LOG pRawLog){
    PHOIT_LOG_INFO          pLogInfo;

    PHOIT_LOG_SECTOR        pLogSector;
    PHOIT_ERASABLE_SECTOR   pErasableLogSector;
    PHOIT_ERASABLE_SECTOR   pErasableSectorTraverse;

    pLogInfo = (PHOIT_LOG_INFO)lib_malloc(sizeof(HOIT_LOG_INFO));
    lib_memset(pLogInfo, 0, sizeof(HOIT_LOG_INFO));

    pLogSector = (PHOIT_LOG_SECTOR)lib_malloc(sizeof(HOIT_LOG_SECTOR));
    lib_memset(pLogInfo, 0, sizeof(HOIT_LOG_SECTOR));
    
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
    pLogSector->pErasableNextLogSector = LW_NULL;
    lib_memcpy(&pLogSector->ErasableSetcor, pErasableLogSector, sizeof(HOIT_LOG_SECTOR));
    
    pLogInfo->pLogSectorList = pLogSector;
    
}

VOID hoitLogRead(PHOIT_VOLUME pfs, UINT uiOfs, PCHAR pLog, UINT uiSize){
    
}

VOID hoitLogAppend(PHOIT_VOLUME pfs, PCHAR pLog){

}

BOOL hoitLogCheckIfLog(PHOIT_VOLUME pfs, PHOIT_ERASABLE_SECTOR pErasableSector){
    PHOIT_LOG_SECTOR    pLogSectorTraverse;
    pLogSectorTraverse = pfs->HOITFS_logInfo->pLogSectorList;
    while (pLogSectorTraverse)
    {
        if(!lib_memcmp(&pLogSectorTraverse->ErasableSetcor, pErasableSector, sizeof(HOIT_ERASABLE_SECTOR))){
            return LW_TRUE;
        }
        pLogSectorTraverse = pLogSectorTraverse->pErasableNextLogSector;
    }
    return LW_FALSE;
}