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
** 文   件   名: hoitFsGC.c
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 04 月 25 日
**
** 描        述: 垃圾回收实现
*********************************************************************************************************/
#include "hoitFsGC.h"
#include "hoitFsCache.h"
#include "hoitFsFDLib.h"
#include "hoitFsLib.h"


typedef enum hoitGCLevel
{
    GC_FOREGROUND,
    GC_BACKGROUND
} ENUM_HOIT_GC_LEVEL;


/*********************************************************************************************************
** 函数名称: __hoitFsGCSectorRawInfoFixUp
** 功能描述: 释放所有pErasableSector中的过期RawInfo，修改next_phys关系
** 输　入  : pErasableSector        目标擦除块
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __hoitFsGCSectorRawInfoFixUp(PHOIT_ERASABLE_SECTOR pErasableSector){
    PHOIT_RAW_INFO          pRawInfoFirst;
    PHOIT_RAW_INFO          pRawInfoLast;
    
    PHOIT_RAW_INFO          pRawInfoTrailing;               /* 前一个指针 */
    PHOIT_RAW_INFO          pRawInfoTraverse;               /* 当前指针 */
    PHOIT_RAW_INFO          pRawInfoObselete;


    pRawInfoTrailing    = LW_NULL;
    pRawInfoTraverse    = pErasableSector->HOITS_pRawInfoFirst;
    
    while (!pRawInfoTraverse->is_obsolete)                  /* 寻找第一个非obselete的RawInfo，并释放已过期的RawInfo */
    {
        pRawInfoObselete = pRawInfoTraverse;
        pRawInfoTraverse = pRawInfoTraverse->next_phys;
        lib_free(pRawInfoObselete);
    }

    pRawInfoFirst    = pRawInfoTraverse;                    /* 设置RawInfo First */
    pRawInfoTrailing = pRawInfoTraverse;                    
    pRawInfoTraverse = pRawInfoTraverse->next_phys;
    
    while (LW_TRUE)
    {
        if(pRawInfoTrailing == pErasableSector->HOITS_pRawInfoLast){    /* 扫描完毕 */
            break;
        }
        if(pRawInfoTraverse->is_obsolete){                              /* 如果过期 */
            pRawInfoObselete            = pRawInfoTraverse;             
            pRawInfoTrailing->next_phys = pRawInfoTraverse->next_phys;  /* 修改指针——前一块指向当前块的下一块 */
            pRawInfoTraverse            = pRawInfoTraverse->next_phys;  /* 置当前块为下一块 */
            lib_free(pRawInfoObselete);                                 /* 释放过期的块 */
        }
        else {
            pRawInfoLast                = pRawInfoTraverse;             /* 更新pRawInfoLast */
            pRawInfoTrailing            = pRawInfoTraverse;              
            pRawInfoTraverse            = pRawInfoTraverse->next_phys;
        }
    }

    pErasableSector->HOITS_pRawInfoFirst    = pRawInfoFirst;
    pErasableSector->HOITS_pRawInfoLast     = pRawInfoLast;
}
/*********************************************************************************************************
** 函数名称: __hoitFsGCFindErasableSector
** 功能描述: 根据HoitFS设备头中的信息，寻找一个可擦除数据块，目前寻找FreeSize最小的作为待GC
** 输　入  : pfs        HoitFS文件设备头
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_ERASABLE_SECTOR __hoitFsGCFindErasableSector(PHOIT_VOLUME pfs, ENUM_HOIT_GC_LEVEL gcLevel){
    PHOIT_ERASABLE_SECTOR       pErasableVictimSector;
    PHOIT_ERASABLE_SECTOR       pErasableList;
    PHOIT_ERASABLE_SECTOR       pErasableListTraverse;
    
    UINT                        uiMinVictimSan;            /* 最小受害者San值 */
    UINT                        uiVictimSan;               /* 受害者San值，越小越受害 */

    UINT                        uiFreeSize;
    UINT                        uiAge;

    uiMinVictimSan              = INT_MAX;

    pErasableList               = pfs->HOITFS_erasableSectorList;
    pErasableListTraverse       = pErasableList;
    
    while (pErasableListTraverse)
    {
        uiFreeSize  = pErasableListTraverse->HOITS_uiFreeSize;   
        switch (gcLevel)
        {
        case GC_BACKGROUND:{
            uiAge       = API_TimeGet() - pErasableListTraverse->HOITS_tBornAge;
            uiVictimSan = (0.5 / uiAge) + 0.5 * uiFreeSize;  
        }
            break;
        case GC_FOREGROUND:{
            uiVictimSan = uiFreeSize;
        }
            break;
        default:
            break;
        }

        if(uiVictimSan < uiMinVictimSan){
            pErasableVictimSector   = pErasableListTraverse;
            uiMinVictimSan          = uiVictimSan;
        }

        pErasableListTraverse   = pErasableListTraverse->HOITS_next;
    }

    return pErasableVictimSector;
}
/*********************************************************************************************************
** 函数名称: __hoitFSGCCollectSetcorAlive
** 功能描述: 从pErasableSector的pRawInfoCurGC处起，获取有效信息，一次获取一个
** 输　入  : pfs        HoitFS文件设备头
** 输　出  : 完成对该Sector的GC  LW_TRUE， 否则LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL __hoitFsGCCollectSetcorAlive(PHOIT_VOLUME pfs, PHOIT_ERASABLE_SECTOR pErasableSector){
    BOOL                    bIsCollectOver;
    
    PHOIT_RAW_INFO          pRawInfoCurGC;
    PHOIT_RAW_INFO          pRawInfoLast;
    
    PCHAR                   pContent;
    
    PHOIT_RAW_HEADER        pRawHeader;

    pRawInfoCurGC   = pErasableSector->HOITS_pRawInfoCurGC;
    pRawInfoLast    = pErasableSector->HOITS_pRawInfoLast;
    bIsCollectOver  = LW_FALSE;

    if(pRawInfoCurGC == LW_NULL){
        pRawInfoCurGC = pErasableSector->HOITS_pRawInfoCurGC = pErasableSector->HOITS_pRawInfoFirst;
    }
    
    //!把RawInfo及其对应的数据实体搬家

    __hoit_move_home(pfs, pRawInfoCurGC);
    pErasableSector->HOITS_uiUsedSize -= pRawInfoCurGC->totlen;
    pErasableSector->HOITS_uiFreeSize += pRawInfoCurGC->totlen;

    if(pRawInfoCurGC == pErasableSector->HOITS_pRawInfoLast){
        bIsCollectOver = LW_TRUE;
    }
    return bIsCollectOver;
}


/*********************************************************************************************************
** 函数名称: hoitFSGCForgroudForce
** 功能描述: HoitFS前台强制GC，Greedy算法
** 输　入  : pfs        HoitFS文件设备头
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID hoitFsGCForgroudForce(PHOIT_VOLUME pfs){
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    INTREG                  iregInterLevel;
    BOOL                    bIsCollectOver;
    
    if(pfs->HOITFS_curGCSector == LW_NULL)
        pErasableSector = __hoitFsGCFindErasableSector(pfs, GC_FOREGROUND);

    while (LW_TRUE)
    {
        if(pErasableSector) {
#ifdef GC_DEBUG
        printf("[%s]: find a Sector start at %ld\n", __func__, pErasableSector->HOITS_offset);
#endif // GC_DEBUG
            LW_SPIN_LOCK_QUICK(&pfs->HOITFS_GCLock, &iregInterLevel);
            __hoitFsGCSectorRawInfoFixUp(pErasableSector);
            pfs->HOITFS_curGCSector = pErasableSector;
            bIsCollectOver = __hoitFsGCCollectSetcorAlive(pfs, pErasableSector);
            if(bIsCollectOver){
                pfs->HOITFS_curGCSector = LW_NULL;
                break;
            }
            LW_SPIN_UNLOCK_QUICK(&pfs->HOITFS_GCLock, &iregInterLevel);
        }
        else {
#ifdef GC_DEBUG
            printf("[%s]: there's no sector can be GCed\n", __func__);
#endif // GC_DEBUG
            break;
        }
    }
}

/*********************************************************************************************************
** 函数名称: hoitFsGCBackgroundThread
** 功能描述: HoitFS后台GC线程
** 输　入  : pfs        HoitFS文件设备头
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID hoitFsGCBackgroundThread(PHOIT_VOLUME pfs){
    INTREG                iregInterLevel;
    PHOIT_ERASABLE_SECTOR pErasableSector;
    BOOL                  bIsCollectOver;
    
    while (LW_TRUE)
    {
        if(pfs->HOITFS_curGCSector == LW_NULL)
            pErasableSector = __hoitFsGCFindErasableSector(pfs, GC_BACKGROUND);

        if(pErasableSector) {
#ifdef GC_DEBUG
            printf("[%s]: find a Sector start at %ld\n", __func__, pErasableSector->HOITS_offset);
#endif // GC_DEBUG
            LW_SPIN_LOCK_QUICK(&pfs->HOITFS_GCLock, &iregInterLevel);
            __hoitFsGCSectorRawInfoFixUp(pErasableSector);
            pfs->HOITFS_curGCSector = pErasableSector;
            bIsCollectOver = __hoitFsGCCollectSetcorAlive(pfs, pErasableSector);
            if(bIsCollectOver){
                pfs->HOITFS_curGCSector = LW_NULL;
            }
            LW_SPIN_UNLOCK_QUICK(&pfs->HOITFS_GCLock, &iregInterLevel);
        }
        else {
#ifdef GC_DEBUG
            printf("[%s]: there's no sector can be GCed\n", __func__);
#endif // GC_DEBUG
        }
        sleep(5);   
    }
}

/*********************************************************************************************************
** 函数名称: hoitFsGCBackgroundThread
** 功能描述: HoitFS后台GC线程
** 输　入  : pfs        HoitFS文件设备头
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID hoitFsGCThread(PHOIT_VOLUME pfs){

}