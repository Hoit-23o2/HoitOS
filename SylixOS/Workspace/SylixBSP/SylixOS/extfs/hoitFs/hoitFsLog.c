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
/*********************************************************************************************************
** 函数名称: __hoitInsertLogSector
** 功能描述: 向pLogSectorList插入一个新的LogSector节点
** 输　入  : pfs            HoitFS设备头
**          pLogSectorNew   新的节点
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __hoitInsertLogSector(PHOIT_VOLUME pfs, PHOIT_LOG_SECTOR pLogSectorNew){
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
/*********************************************************************************************************
** 函数名称: __hoitScanLogSector
** 功能描述: 根据pRawLog计算当前Log Sector中的写偏移
** 输　入  : pfs            HoitFS设备头
**          pRawLog         Flash上的pRawLog数据实体
**          puiEntityNum    记录实体数量
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT __hoitScanLogSector(PHOIT_VOLUME pfs, PHOIT_RAW_LOG pRawLog, UINT * puiEntityNum){
    PCHAR               pcLogSector;
    UINT                uiSectorNum;
    UINT                uiSectorSize;
    UINT                uiOfs;
    PCHAR               pcCurSectorPos;
    PHOIT_RAW_HEADER    pRawHeader;
    PHOIT_RAW_INFO      pRawInfo;

    uiSectorNum     = hoitGetSectorNo(pRawLog->uiLogFirstAddr);
    uiSectorSize    = hoitGetSectorSize(uiSectorNum);
    uiOfs           = 0;

    pcLogSector = (PCHAR)lib_malloc(uiSectorSize);
    hoitReadFromCache(pfs->HOITFS_cacheHdr, pRawLog->uiLogFirstAddr, pcLogSector, uiSectorSize);
    pcCurSectorPos = pcLogSector;

    while (pcCurSectorPos < pcLogSector + uiSectorSize) {
        PHOIT_RAW_HEADER pRawHeader = (PHOIT_RAW_HEADER)pcCurSectorPos;
        if (pRawHeader->magic_num == HOIT_MAGIC_NUM 
            && __HOIT_IS_TYPE_LOG(pRawHeader)) {
            
            /* 将初始的pRawLog对应的RawInfo加入到管理中 */
            pRawInfo                = (PHOIT_RAW_INFO)lib_malloc(sizeof(HOIT_RAW_INFO));
            pRawInfo->phys_addr     = pcCurSectorPos;
            pRawInfo->totlen        = pRawLog->totlen;
            pRawInfo->is_obsolete   = 0;
            pRawInfo->next_logic    = LW_NULL;
            pRawInfo->next_phys     = LW_NULL;
            __hoit_add_raw_info_to_sector(pfs->HOITFS_now_sector, pRawInfo); 
            
            pcCurSectorPos += __HOIT_MIN_4_TIMES(pRawHeader->totlen);
            uiOfs = (pcCurSectorPos - pcLogSector); 
            *puiEntityNum = *puiEntityNum + 1;
        }
        else {
            pcCurSectorPos += 4;   /* 每次移动4字节 */
        }
    }
    return uiOfs;
}
/*********************************************************************************************************
** 函数名称: __hoitFindAvailableSector
** 功能描述: 寻找一个全空的Sector
** 输　入  : pfs            HoitFS设备头
** 输　出  : 一个全空的Sector
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_ERASABLE_SECTOR __hoitFindAvailableSector(PHOIT_VOLUME pfs){
    UINT                    uiAvaiSectorNum;
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    
    uiAvaiSectorNum = hoitFindNextToWrite(pfs->HOITFS_cacheHdr, HOIT_CACHE_TYPE_DATA_EMPTY);
    if(uiAvaiSectorNum == PX_ERROR){
        return LW_NULL;
    }
    

    return pErasableSector;
}
/*********************************************************************************************************
** 函数名称: hoitLogInit
** 功能描述: Build的时候，如果没有扫描到LOG，就调用InitLOG，初始化一个日志系统
** 输　入  : pfs            HoitFS设备头
**          uiLogSize       Log大小
**          uiSectorNum     Log Sector数量，暂时只支持1
** 输　出  : LOG文件信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_LOG_INFO hoitLogInit(PHOIT_VOLUME pfs, UINT uiLogSize, UINT uiSectorNum){
    PHOIT_RAW_INFO          pRawInfo;
    PHOIT_RAW_LOG           pRawLog;
    PHOIT_LOG_INFO          pLogInfo;
    
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    PHOIT_LOG_SECTOR        pLogSector;

    UINT                    uiSectorAddr;

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

    uiSectorAddr = hoitWriteToCache(pfs->HOITFS_cacheHdr, (PCHAR)pRawLog, pRawLog->totlen);
    
    /* 将初始的pRawLog对应的RawInfo加入到管理中 */
    pRawInfo                = (PHOIT_RAW_INFO)lib_malloc(sizeof(HOIT_RAW_INFO));
    pRawInfo->phys_addr     = uiSectorAddr;
    pRawInfo->totlen        = pRawLog->totlen;
    pRawInfo->is_obsolete   = 0;
    pRawInfo->next_logic    = LW_NULL;
    pRawInfo->next_phys     = LW_NULL;
    __hoit_add_raw_info_to_sector(pfs->HOITFS_now_sector, pRawInfo); 

    pLogInfo = (PHOIT_LOG_INFO)lib_malloc(sizeof(HOIT_LOG_INFO));
    lib_memset(pLogInfo, 0, sizeof(HOIT_LOG_INFO));
    
    pLogSector = (PHOIT_LOG_SECTOR)lib_malloc(sizeof(HOIT_LOG_SECTOR));
    lib_memset(pLogSector, 0, sizeof(HOIT_LOG_SECTOR));
    lib_memcpy(&pLogSector->ErasableSetcor,  pErasableSector, sizeof(HOIT_ERASABLE_SECTOR));
    
    pLogSector->pErasableNextLogSector = LW_NULL;

    pLogInfo->pLogSectorList           = pLogSector;
    pLogInfo->uiLogCurAddr             = pRawLog->uiLogFirstAddr;
    pLogInfo->uiLogCurOfs              = 0;
    pLogInfo->uiLogEntityCnt           = 0;
    pLogInfo->uiLogSize                = uiLogSize;
    
    pfs->HOITFS_logInfo                = pLogInfo;

    return pLogInfo;
}

/*********************************************************************************************************
** 函数名称: hoitLogOpen
** 功能描述: Build的时候，如果扫描到 有效 LOG，就调用hoitLogOpen，从而初始化LOG系统
** 输　入  : pfs            HoitFS设备头
**          pRawLog        Flash上的RAW_LOG信息 
** 输　出  : LOG文件信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_LOG_INFO hoitLogOpen(PHOIT_VOLUME pfs, PHOIT_RAW_LOG pRawLog){
    PHOIT_LOG_INFO          pLogInfo;

    PHOIT_LOG_SECTOR        pLogSector;
    PHOIT_ERASABLE_SECTOR   pErasableLogSector;
    PHOIT_ERASABLE_SECTOR   pErasableSectorTraverse;

    UINT                    uiEntityCnt;
    UINT                    uiLogCurOfs;
    
    uiEntityCnt = 0;

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
    uiLogCurOfs = __hoitScanLogSector(pfs, pRawLog, &uiEntityCnt);

    pLogSector->pErasableNextLogSector = LW_NULL;
    lib_memcpy(&pLogSector->ErasableSetcor, pErasableLogSector, sizeof(HOIT_LOG_SECTOR));
    pLogInfo->pLogSectorList = pLogSector;
    pLogInfo->uiLogCurAddr   = pRawLog->uiLogFirstAddr;
    pLogInfo->uiLogCurOfs    = uiLogCurOfs;
    pLogInfo->uiLogSize      = pRawLog->uiLogSize;
    pLogInfo->uiLogEntityCnt = uiEntityCnt;
    
    pfs->HOITFS_logInfo = pLogInfo;
    return pLogInfo;
}
/*********************************************************************************************************
** 函数名称: hoitLogRead
** 功能描述: 读取Log Sector上的信息
** 输　入  : pfs            HoitFS设备头
**          uiEntityNum      该Sector上第uiEntityNum个实体的信息
** 输　出  : LOG文件信息
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT hoitLogEntityGet(PHOIT_VOLUME pfs, UINT uiEntityNum){
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    PHOIT_RAW_INFO          pRawInfoTraverse;
    UINT                    uiEntityIndex;

    if(uiEntityNum > pfs->HOITFS_logInfo->uiLogEntityCnt){
#ifdef LOG_DEBUG
        printf("[%s] uiEntityNum start from 0, Log has no enough entities to get\n",__func__);
#endif // LOG_DEBUG
        return PX_ERROR;
    }

    uiEntityIndex = 0;
    pErasableSector = &pfs->HOITFS_logInfo->pLogSectorList->ErasableSetcor;     /* 获取Log Sector */
    pRawInfoTraverse = pErasableSector->HOITS_pRawInfoFirst;

    while (LW_TRUE)
    {
        if(uiEntityIndex == uiEntityNum){
            break;
        }
        uiEntityIndex++;
        pRawInfoTraverse = pRawInfoTraverse->next_phys;
    }

    /* 目标 Entity 所在RawInfo */
    
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