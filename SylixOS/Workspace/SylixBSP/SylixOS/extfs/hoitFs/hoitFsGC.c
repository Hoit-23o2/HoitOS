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


    pRawInfoFirst       = LW_NULL;
    pRawInfoLast        = LW_NULL;

    pRawInfoTrailing    = LW_NULL;
    pRawInfoTraverse    = pErasableSector->HOITS_pRawInfoFirst;
    
    while (pRawInfoTraverse->is_obsolete)                               /* 寻找第一个非obselete的RawInfo，并释放已过期的RawInfo */
    {
        pRawInfoObselete = pRawInfoTraverse;
        pRawInfoTraverse = pRawInfoTraverse->next_phys;
        lib_free(pRawInfoObselete);
    }

    pRawInfoFirst    = pRawInfoLast = pRawInfoTraverse;                 /* 设置RawInfo First与RawInfo Last */
    pRawInfoTrailing = pRawInfoTraverse;                    
    pRawInfoTraverse = pRawInfoTraverse->next_phys;
    
    while (LW_TRUE)
    {
        if(pRawInfoTrailing == pErasableSector->HOITS_pRawInfoLast
         ||pRawInfoTraverse == LW_NULL){    /* 扫描完毕 */
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
    PHOIT_ERASABLE_SECTOR       pErasableListTraverse;
    
    UINT                        uiMinVictimSan;            /* 最小受害者San值 */
    UINT                        uiVictimSan;               /* 受害者San值，越小越受害 */

    UINT                        uiFreeSize;
    UINT                        uiAge;

    pErasableVictimSector       = LW_NULL;
    uiMinVictimSan              = INT_MAX;

    pErasableListTraverse       = pfs->HOITFS_erasableSectorList;
    
    while (pErasableListTraverse)
    {
        uiFreeSize  = pErasableListTraverse->HOITS_uiFreeSize;   
#ifdef GC_TEST
        if(pErasableListTraverse->HOITS_next == LW_NULL){
            pErasableListTraverse->HOITS_uiFreeSize -= 3;
            pErasableListTraverse->HOITS_uiUsedSize += 3;
        }
#endif // GC_TEST
        if(pErasableListTraverse->HOITS_uiUsedSize == 0){
            pErasableListTraverse   = pErasableListTraverse->HOITS_next;
            continue;
        }

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
    PHOIT_RAW_INFO          pRawInfoNextGC;
    
    pRawInfoCurGC   = pErasableSector->HOITS_pRawInfoCurGC;
    pRawInfoNextGC  = pRawInfoCurGC->next_phys;

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

    pErasableSector->HOITS_pRawInfoCurGC = pRawInfoNextGC;
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
VOID hoitGCForgroudForce(PHOIT_VOLUME pfs){
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    PHOIT_RAW_INFO          pNextGCRawInfo;
    INTREG                  iregInterLevel;
    BOOL                    bIsCollectOver;

    
    if(pfs->HOITFS_curGCSector == LW_NULL)
        pErasableSector = __hoitFsGCFindErasableSector(pfs, GC_FOREGROUND);

    while (LW_TRUE)
    {
        if(pErasableSector) {
#ifdef GC_DEBUG
        API_ThreadLock();
        printf("[%s]: find a Sector, which no. is %d\n", __func__, pErasableSector->HOITS_bno);
        API_ThreadUnlock();
#endif // GC_DEBUG
            LW_SPIN_LOCK_QUICK(&pfs->HOITFS_GCLock, &iregInterLevel);
            __hoitFsGCSectorRawInfoFixUp(pErasableSector);
            pfs->HOITFS_curGCSector = pErasableSector;
            bIsCollectOver = __hoitFsGCCollectSetcorAlive(pfs, pErasableSector);
            if(bIsCollectOver){
                pfs->HOITFS_curGCSector = LW_NULL;
                break;
            }
            LW_SPIN_UNLOCK_QUICK(&pfs->HOITFS_GCLock, iregInterLevel);
        }
        else {
#ifdef GC_DEBUG
            API_ThreadLock();
            printf("[%s]: there's no sector can be GCed\n", __func__);
            API_ThreadUnlock();
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
VOID hoitGCBackgroundThread(PHOIT_VOLUME pfs){
    INTREG                iregInterLevel;
    PHOIT_ERASABLE_SECTOR pErasableSector;
    BOOL                  bIsCollectOver;
    CHAR                  acMsg[MAX_MSG_BYTE_SIZE];
    size_t                stLen;

    while (LW_TRUE)
    {
        API_MsgQueueReceive(pfs->HOITFS_GCMsgQ, 
                            acMsg, 
                            sizeof(acMsg), 
                            &stLen, 
                            5);
        if(lib_strcmp(acMsg, MSG_BG_GC_END)){
            break;
        }
        if(pfs->HOITFS_curGCSector == LW_NULL)
            pErasableSector = __hoitFsGCFindErasableSector(pfs, GC_BACKGROUND);

        if(pErasableSector) {
#ifdef GC_DEBUG
            API_ThreadLock();
            printf("[%s]: find a Sector start at %d\n", __func__, pErasableSector->HOITS_offset);
            API_ThreadUnlock();
#endif // GC_DEBUG
            LW_SPIN_LOCK_QUICK(&pfs->HOITFS_GCLock, &iregInterLevel);
            __hoitFsGCSectorRawInfoFixUp(pErasableSector);
            pfs->HOITFS_curGCSector = pErasableSector;
            bIsCollectOver = __hoitFsGCCollectSetcorAlive(pfs, pErasableSector);
            if(bIsCollectOver){
                pfs->HOITFS_curGCSector = LW_NULL;
            }
            LW_SPIN_UNLOCK_QUICK(&pfs->HOITFS_GCLock, iregInterLevel);
        }
        else {
#ifdef GC_DEBUG
            API_ThreadLock();
            printf("[%s]: there's no sector can be GCed\n", __func__);
            API_ThreadUnlock();
#endif // GC_DEBUG
        }
    }
}

/*********************************************************************************************************
** 函数名称: hoitFsGCThread
** 功能描述: HoitFS GC线程
** 输　入  : pfs        HoitFS文件设备头
**          uiThreshold GC阈值，参考F2FS
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID hoitGCThread(PHOIT_GC_ATTR pGCAttr){
    PHOIT_VOLUME                pfs; 
    UINT                        uiThreshold;
    UINT                        uiCurUsedSize;
    BOOL                        bIsBackgroundThreadStart;
    LW_CLASS_THREADATTR         gcThreadAttr;
    LW_OBJECT_HANDLE            hGcThreadId;

    bIsBackgroundThreadStart = LW_FALSE;
    pfs                      = pGCAttr->pfs;
    uiThreshold              = pGCAttr->uiThreshold;

    printf("\n\n[GC Thread Start...]\n\n");
    pfs->HOITFS_GCMsgQ = API_MsgQueueCreate("q_gc_thread_msgq", 
                                            MAX_MSG_COUNTER, 
                                            MAX_MSG_BYTE_SIZE, 
                                            LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_LOCAL, 
                                            LW_NULL);
#ifdef GC_TEST
    uiCurUsedSize = 60;
#endif // GC_TEST

    while (LW_TRUE)
    {
        uiCurUsedSize = pfs->HOITFS_totalUsedSize;
        if(uiCurUsedSize > uiThreshold){                                    /* 执行Foreground */
            if(bIsBackgroundThreadStart){
                API_MsgQueueSend(pfs->HOITFS_GCMsgQ, MSG_BG_GC_END, sizeof(MSG_BG_GC_END));
                API_ThreadJoin(hGcThreadId, LW_NULL);
                bIsBackgroundThreadStart = LW_FALSE;
            }
            hoitGCForgroudForce(pfs);
        }
        else {
            if(!bIsBackgroundThreadStart 
                && uiCurUsedSize > (pfs->HOITFS_totalSize / 2)){            /* 执行Background */
                
                bIsBackgroundThreadStart = LW_TRUE;
                
                API_ThreadAttrBuild(&gcThreadAttr, 
                                    4 * LW_CFG_KB_SIZE, 
                                    LW_PRIO_NORMAL,
                                    LW_OPTION_THREAD_STK_CHK, 
                                    (VOID *)pfs);

                hGcThreadId = API_ThreadCreate("t_gc_background_thread",
						                       (PTHREAD_START_ROUTINE)hoitGCBackgroundThread,
        						               &gcThreadAttr,
		        				               LW_NULL);
            }
        }
        sleep(10);
    }
}
