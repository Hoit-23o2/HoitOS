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
/*********************************************************************************************************
** 函数名称: __hoitFsGCFindErasableSector
** 功能描述: 根据HoitFS设备头中的信息，寻找一个可擦除数据块，目前寻找FreeSize最小的作为待GC
** 输　入  : pfs        HoitFS文件设备头
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_ERASABLE_SECTOR __hoitFsGCFindErasableSector(PHOIT_VOLUME pfs){
    PHOIT_ERASABLE_SECTOR       pErasableSector;
    PHOIT_ERASABLE_SECTOR       pErasableList;
    PHOIT_ERASABLE_SECTOR       pErasableListTraverse;
    
    UINT                        uiMinFreeSize;
    UINT                        uiFreeSize;

    uiMinFreeSize           = INT_MAX;
    pErasableList           = pfs->HOITFS_erasableSectorList;
    pErasableListTraverse   = pErasableList;
    
    while (pErasableListTraverse)
    {
        uiFreeSize            = pErasableListTraverse->HOITS_uiFreeSize;

        if(uiFreeSize < uiMinFreeSize){
            pErasableSector   = pErasableListTraverse;
            uiMinFreeSize     = uiFreeSize;
        }

        pErasableListTraverse = pErasableListTraverse->HOITS_next;
    }

    return pErasableSector;
}

VOID __hoitFsGCCollectDNode(PHOIT_VOLUME pfs, PHOIT_ERASABLE_SECTOR pErasableSector){

}

/*********************************************************************************************************
** 函数名称: __hoitFSGCCollectSetcorAlive
** 功能描述: 从pErasableSector的pRawInfoCurGC处起，获取有效信息，一次获取一个
** 输　入  : pfs        HoitFS文件设备头
** 输　出  : 完成对该Sector的GC  LW_TRUE， 否则LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL __hoitFSGCCollectSetcorAlive(PHOIT_VOLUME pfs, PHOIT_ERASABLE_SECTOR pErasableSector){
    BOOL                    bIsCollectOver;
    
    PHOIT_RAW_INFO          pRawInfoCurGC;
    PHOIT_RAW_INFO          pRawInfoLast;
    
    PCHAR                   pContent;
    
    PHOIT_RAW_HEADER        pRawHeader;        
    PHOIT_RAW_DIRENT        pRawDirent;
    PHOIT_RAW_INODE         pRawInode;

    UINT                    uiCurSectorNo;
    UINT                    uiCurSectorOfs;

    pRawInfoCurGC   = pErasableSector->HOITS_pRawInfoCurGC;
    pRawInfoLast    = pErasableSector->HOITS_pRawInfoLast;
    bIsCollectOver  = LW_FALSE;

    if(pRawInfoCurGC == LW_NULL){
        pRawInfoCurGC = pErasableSector->HOITS_pRawInfoCurGC = pErasableSector->HOITS_pRawInfoFirst;
    }

    //TODO:在这里分析
    pContent = (PCHAR)lib_malloc(pRawInfoCurGC->totlen);
    hoitReadFromCache(pRawInfoCurGC->phys_addr, pContent, pRawInfoCurGC->totlen);
    pRawHeader = (PHOIT_RAW_HEADER)pContent;
    
    uiCurSectorNo   = pfs->HOITFS_now_sector->HOITS_bno;
    uiCurSectorOfs  = pfs->HOITFS_now_sector->HOITS_offset;
    
    if(!__HOIT_IS_OBSOLETE(pRawHeader)){
        if(__HOIT_IS_TYPE_DIRENT(pRawHeader)){
            pRawDirent = (PHOIT_RAW_INODE)pContent;
            //TODO:Cache层写的时候会更新now_sector
            
        }
        else if (__HOIT_IS_TYPE_INODE(pRawHeader))
        {
            pRawInode = (PHOIT_RAW_INODE)pContent;
            
        }
    }

    if(pRawInfoCurGC == pErasableSector->HOITS_pRawInfoLast){
        bIsCollectOver = LW_TRUE;
    }
    return bIsCollectOver;
}
/*********************************************************************************************************
** 函数名称: hoitFSGCForgroudForce
** 功能描述: HoitFS前台强制GC
** 输　入  : pfs        HoitFS文件设备头
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID hoitFSGCForgroudForce(PHOIT_VOLUME pfs){

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
    
    while (LW_TRUE)
    {
        if(pfs->HOITFS_curGCSector == LW_NULL)
            pErasableSector = __hoitFsGCFindErasableSector(pfs);

        if(pErasableSector) {
#ifdef GC_DEBUG
            printf("[%s]: find a Sector start at %ld\n", __func__, pErasableSector->HOITS_offset);
#endif // GC_DEBUG
            LW_SPIN_LOCK_QUICK(&pfs->HOITFS_GCLock, &iregInterLevel);
            pfs->HOITFS_curGCSector = pErasableSector;
            __hoitFSGCCollectSetcorAlive(pfs, pErasableSector);
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