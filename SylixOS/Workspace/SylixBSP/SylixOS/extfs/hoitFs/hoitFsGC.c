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
** 函数名称: __hoitGCSectorRawInfoFixUp
** 功能描述: 释放所有pErasableSector中的过期RawInfo，修改next_phys关系
** 输　入  : pErasableSector        目标擦除块
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __hoitGCSectorRawInfoFixUp(PHOIT_ERASABLE_SECTOR pErasableSector){
    PHOIT_RAW_INFO          pRawInfoFirst;
    PHOIT_RAW_INFO          pRawInfoLast;
    
    PHOIT_RAW_INFO          pRawInfoTrailing;               /* 前一个指针 */
    PHOIT_RAW_INFO          pRawInfoTraverse;               /* 当前指针 */
    PHOIT_RAW_INFO          pRawInfoObselete;

    BOOL                    bIsReset;

    pRawInfoFirst       = LW_NULL;
    pRawInfoLast        = LW_NULL;

    pRawInfoTrailing    = LW_NULL;
    pRawInfoTraverse    = pErasableSector->HOITS_pRawInfoFirst;
    
    bIsReset = LW_FALSE;

    if((pErasableSector->HOITS_pRawInfoCurGC 
    && pErasableSector->HOITS_pRawInfoCurGC->is_obsolete) 
    || pErasableSector->HOITS_pRawInfoCurGC == LW_NULL){                /* 如果当前GC RawInfo过期，或还不存在当前GC RawInfo，则重新开始RawInfo的GC */
        bIsReset = LW_TRUE;
    }
    
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
        || pRawInfoTraverse == LW_NULL){    /* 扫描完毕 */
            break;
        }
        if(pRawInfoTraverse->is_obsolete){                                      /* 如果过期 */
            pRawInfoObselete                    = pRawInfoTraverse;             
            pRawInfoTrailing->next_phys         = pRawInfoTraverse->next_phys;  /* 修改指针——前一块指向当前块的下一块 */
            pRawInfoTraverse                    = pRawInfoTraverse->next_phys;  /* 置当前块为下一块 */
            
            pErasableSector->HOITS_uiUsedSize   -= pRawInfoObselete->totlen;
            pErasableSector->HOITS_uiFreeSize   += pRawInfoObselete->totlen;
            
            lib_free(pRawInfoObselete);                                         /* 释放过期的块 */
        }
        else {
            pRawInfoLast                = pRawInfoTraverse;                     /* 更新pRawInfoLast */
            pRawInfoTrailing            = pRawInfoTraverse;              
            pRawInfoTraverse            = pRawInfoTraverse->next_phys;
        }
    }

    pErasableSector->HOITS_pRawInfoFirst    = pRawInfoFirst;
    pErasableSector->HOITS_pRawInfoLast     = pRawInfoLast;

    if(bIsReset){                                                   /* 初始化 Last GC， Info CurGC, Info Prev GC */
        pErasableSector->HOITS_pRawInfoCurGC    = LW_NULL;
        pErasableSector->HOITS_pRawInfoPrevGC   = LW_NULL;
        if(pErasableSector->HOITS_pRawInfoLastGC == LW_NULL){
            pErasableSector->HOITS_pRawInfoLastGC = (PHOIT_RAW_INFO)lib_malloc(sizeof(HOIT_RAW_INODE));
        }
        if(pRawInfoLast){
            lib_memcpy(pErasableSector->HOITS_pRawInfoLastGC, pRawInfoLast, sizeof(HOIT_RAW_INFO));
        }
        else {
            lib_free(pErasableSector->HOITS_pRawInfoLastGC);
            pErasableSector->HOITS_pRawInfoLastGC = LW_NULL;        /* 该Sector中没有Raw Info了…… */
        }
    }
}
/*********************************************************************************************************
** 函数名称: __hoitGCFindErasableSector
** 功能描述: 根据HoitFS设备头中的信息，寻找一个可擦除数据块，目前寻找FreeSize最小的作为待GC
** 输　入  : pfs        HoitFS文件设备头
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_ERASABLE_SECTOR __hoitGCFindErasableSector(PHOIT_VOLUME pfs, ENUM_HOIT_GC_LEVEL gcLevel){
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
        if(pErasableListTraverse->HOITS_next == LW_NULL){           /* 如果最后一个Sector了 */
            pErasableListTraverse->HOITS_uiFreeSize -= 3;           /* 适配一下 */
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
** 函数名称: __hoitGCCollectRawInfoAlive
** 功能描述: 从pErasableSector的pRawInfoCurGC处起，获取有效信息，一次获取一个
** 输　入  : pfs        HoitFS文件设备头
** 输　出  : 完成对该Sector的GC  LW_TRUE， 否则LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __hoitGCCollectRawInfoAlive(PHOIT_VOLUME pfs, PHOIT_ERASABLE_SECTOR pErasableSector, PHOIT_RAW_INFO pRawInfoCurGC, BOOL *pbIsMoveSuccess){

    if(pErasableSector == LW_NULL){
#ifdef GC_DEBUG
        printf("[%s] setcor can not be null\n", __func__);
#endif // GC_DEBUG
        return LW_TRUE;
    }
    

    //!把RawInfo及其对应的数据实体搬家
    *pbIsMoveSuccess = __hoit_move_home(pfs, pRawInfoCurGC);  /* 搬家失败，说明该RawInfo要么是LOG要么是一段错误的数据，我们直接跳过 */

    if(*pbIsMoveSuccess){
        pErasableSector->HOITS_uiUsedSize -= pRawInfoCurGC->totlen;
        pErasableSector->HOITS_uiFreeSize += pRawInfoCurGC->totlen;

        printf("[%s] GC has released size %d of sector %d\n", 
                __func__, pRawInfoCurGC->totlen, pErasableSector->HOITS_bno);
    }

}

/*********************************************************************************************************
** 函数名称: __hoitGCCollectSectorAlive
** 功能描述: 从pErasableSector的pRawInfoCurGC处起，获取有效信息，一次获取一个，并修改相应指针
** 输　入  : pfs                HoitFS文件设备头
**          pErasableSector     
** 输　出  : 完成对该Sector的GC  LW_TRUE， 否则LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL __hoitGCCollectSectorAlive(PHOIT_VOLUME pfs, PHOIT_ERASABLE_SECTOR pErasableSector){
    BOOL                    bIsCollectOver;
    BOOL                    bIsMoveSuccess;
    INTREG                  iregInterLevel;

    PHOIT_RAW_INFO          pRawInfoNextGC;
    PHOIT_RAW_INFO          pRawInfoCurGC;
    PHOIT_RAW_INFO          pRawInfoPrevGC;

    LW_SPIN_LOCK_QUICK(&pfs->HOITFS_GCLock, &iregInterLevel);
    __hoitGCSectorRawInfoFixUp(pErasableSector);            /* FixUp后，会更新 HOITS_pRawInfoCurGC，HOITS_pRawInfoPrevGC，HOITS_pRawInfoLastGC等 */
    
    printf("[%s]: Fix over the Sector %d\n", __func__, pErasableSector->HOITS_bno);
    pRawInfoCurGC   = LW_NULL;
    pRawInfoPrevGC  = LW_NULL;
    pRawInfoNextGC  = LW_NULL;
    bIsCollectOver  = LW_FALSE;
    bIsMoveSuccess  = LW_FALSE;
    
    pRawInfoCurGC   = pErasableSector->HOITS_pRawInfoCurGC;
    pRawInfoPrevGC  = pErasableSector->HOITS_pRawInfoPrevGC;

    if(pRawInfoCurGC == LW_NULL){
        pRawInfoCurGC = pErasableSector->HOITS_pRawInfoCurGC = pErasableSector->HOITS_pRawInfoFirst;
    }

    pRawInfoNextGC  = pRawInfoCurGC->next_phys;

#ifdef GC_DEBUG
    API_ThreadLock();
    printf("[%s]: GC sector %ld, GC raw info at %u\n", __func__, 
            pErasableSector->HOITS_bno, pRawInfoCurGC->phys_addr);
    API_ThreadUnlock();
#endif // GC_DEBUG

    pfs->HOITFS_curGCSector = pErasableSector;

    if(pErasableSector->HOITS_pRawInfoLastGC == LW_NULL){                                  /* 不存在Last了 */
        bIsCollectOver = LW_TRUE;
    }

    if(pErasableSector->HOITS_pRawInfoLastGC 
    && pErasableSector->HOITS_pRawInfoLastGC->phys_addr == pRawInfoCurGC->phys_addr
    && pErasableSector->HOITS_pRawInfoLastGC->next_logic == pRawInfoCurGC->next_logic
    && pErasableSector->HOITS_pRawInfoLastGC->totlen == pRawInfoCurGC->totlen){             /* 不能比较next_phys和phys，会被修改之 */
        bIsCollectOver = LW_TRUE;
    }

    __hoitGCCollectRawInfoAlive(pfs, pErasableSector, pRawInfoCurGC, &bIsMoveSuccess);


    if(bIsMoveSuccess){                                                  /* 移动成功 */
        if(pRawInfoCurGC == pErasableSector->HOITS_pRawInfoFirst){       /* 如果当前GC块是Sector的第一个RawInfo */
            pErasableSector->HOITS_pRawInfoPrevGC = LW_NULL;             /* Prev = LW_NULL */
            pErasableSector->HOITS_pRawInfoFirst  = pRawInfoNextGC;      /* 重置Sector的第一个RawInfo */
        }
        else {                                                           /* 如果不是第一个RawInfo */
            pRawInfoPrevGC->next_phys = pRawInfoNextGC;                  /* 让前一个RawInfo指向Cur的下一个 */
            if(bIsCollectOver){                                          /* 如果Cur是最后一个RawInfo */
                if(pfs->HOITFS_now_sector == pErasableSector){           /* 如果该RawInfo仍然被写到该Sector */
                    /* Do Nothing */
                }
                else {                                                   /* 如果该RawInfo被写到别的Sector */
                    pRawInfoPrevGC->next_phys = LW_NULL;                 /* 让Last指针指向前一个RawInfo即可 */
                    pErasableSector->HOITS_pRawInfoLast = pRawInfoPrevGC;
                }
            }
        }
    }
    else {                                                               /* 如果没有MOVE成功 */
        pErasableSector->HOITS_pRawInfoPrevGC = pRawInfoCurGC;           /* Prev就是当前的RawInfo */
    }

    pErasableSector->HOITS_pRawInfoCurGC  = pRawInfoNextGC;              /* 调整Cur指针 */
    
    if(bIsCollectOver){
        pErasableSector->HOITS_pRawInfoCurGC  = LW_NULL;                  /* 当前Sector中GC的RawInfo为空 */
        pErasableSector->HOITS_pRawInfoPrevGC = LW_NULL;
        pfs->HOITFS_curGCSector = LW_NULL;                                /* 当前GC的Sector为空 */
        printf("[%s]: Sector %d is collected Over\n", __func__, pErasableSector->HOITS_bno);
    }

    LW_SPIN_UNLOCK_QUICK(&pfs->HOITFS_GCLock, iregInterLevel);

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
    INTREG                  iregInterLevel;
    BOOL                    bIsCollectOver;
    
    if(pfs->HOITFS_curGCSector == LW_NULL) {
        pErasableSector = __hoitGCFindErasableSector(pfs, GC_FOREGROUND);
    }

    while (LW_TRUE)
    {
        if(pErasableSector) {
            bIsCollectOver = __hoitGCCollectSectorAlive(pfs, pErasableSector);
            if(bIsCollectOver){
                break;
            }
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

        if(pfs->HOITFS_curGCSector == LW_NULL) {
            pErasableSector = __hoitGCFindErasableSector(pfs, GC_BACKGROUND);
        }

        if(pErasableSector) {
            bIsCollectOver = __hoitGCCollectSectorAlive(pfs, pErasableSector);
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
