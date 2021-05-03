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
** ��   ��   ��: hoitFsGC.c
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 04 �� 25 ��
**
** ��        ��: ��������ʵ��
*********************************************************************************************************/
#include "hoitFsGC.h"
#include "hoitFsCache.h"
#include "hoitFsFDLib.h"
#include "hoitFsLib.h"

/*********************************************************************************************************
** ��������: __hoitFsGCSectorRawInfoFixUp
** ��������: �ͷ�����pErasableSector�еĹ���RawInfo���޸�next_phys��ϵ
** �䡡��  : pErasableSector        Ŀ�������
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __hoitFsGCSectorRawInfoFixUp(PHOIT_ERASABLE_SECTOR pErasableSector){
    PHOIT_RAW_INFO          pRawInfoFirst;
    PHOIT_RAW_INFO          pRawInfoLast;
    
    PHOIT_RAW_INFO          pRawInfoTrailing;               /* ǰһ��ָ�� */
    PHOIT_RAW_INFO          pRawInfoTraverse;               /* ��ǰָ�� */
    PHOIT_RAW_INFO          pRawInfoObselete;


    pRawInfoFirst       = LW_NULL;
    pRawInfoLast        = LW_NULL;

    pRawInfoTrailing    = LW_NULL;
    pRawInfoTraverse    = pErasableSector->HOITS_pRawInfoFirst;
    
    while (pRawInfoTraverse->is_obsolete)                               /* Ѱ�ҵ�һ����obselete��RawInfo�����ͷ��ѹ��ڵ�RawInfo */
    {
        pRawInfoObselete = pRawInfoTraverse;
        pRawInfoTraverse = pRawInfoTraverse->next_phys;
        lib_free(pRawInfoObselete);
    }

    pRawInfoFirst    = pRawInfoLast = pRawInfoTraverse;                 /* ����RawInfo First��RawInfo Last */
    pRawInfoTrailing = pRawInfoTraverse;                    
    pRawInfoTraverse = pRawInfoTraverse->next_phys;
    
    while (LW_TRUE)
    {
        if(pRawInfoTrailing == pErasableSector->HOITS_pRawInfoLast
         ||pRawInfoTraverse == LW_NULL){    /* ɨ����� */
            break;
        }
        if(pRawInfoTraverse->is_obsolete){                              /* ������� */
            pRawInfoObselete            = pRawInfoTraverse;             
            pRawInfoTrailing->next_phys = pRawInfoTraverse->next_phys;  /* �޸�ָ�롪��ǰһ��ָ��ǰ�����һ�� */
            pRawInfoTraverse            = pRawInfoTraverse->next_phys;  /* �õ�ǰ��Ϊ��һ�� */
            lib_free(pRawInfoObselete);                                 /* �ͷŹ��ڵĿ� */
        }
        else {
            pRawInfoLast                = pRawInfoTraverse;             /* ����pRawInfoLast */
            pRawInfoTrailing            = pRawInfoTraverse;              
            pRawInfoTraverse            = pRawInfoTraverse->next_phys;
        }
    }

    pErasableSector->HOITS_pRawInfoFirst    = pRawInfoFirst;
    pErasableSector->HOITS_pRawInfoLast     = pRawInfoLast;
}
/*********************************************************************************************************
** ��������: __hoitFsGCFindErasableSector
** ��������: ����HoitFS�豸ͷ�е���Ϣ��Ѱ��һ���ɲ������ݿ飬ĿǰѰ��FreeSize��С����Ϊ��GC
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_ERASABLE_SECTOR __hoitFsGCFindErasableSector(PHOIT_VOLUME pfs, ENUM_HOIT_GC_LEVEL gcLevel){
    PHOIT_ERASABLE_SECTOR       pErasableVictimSector;
    PHOIT_ERASABLE_SECTOR       pErasableListTraverse;
    
    UINT                        uiMinVictimSan;            /* ��С�ܺ���Sanֵ */
    UINT                        uiVictimSan;               /* �ܺ���Sanֵ��ԽСԽ�ܺ� */

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
** ��������: __hoitFSGCCollectSetcorAlive
** ��������: ��pErasableSector��pRawInfoCurGC���𣬻�ȡ��Ч��Ϣ��һ�λ�ȡһ��
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
** �䡡��  : ��ɶԸ�Sector��GC  LW_TRUE�� ����LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
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
    
    //!��RawInfo�����Ӧ������ʵ����
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
** ��������: hoitFSGCForgroudForce
** ��������: HoitFSǰ̨ǿ��GC��Greedy�㷨
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: hoitFsGCBackgroundThread
** ��������: HoitFS��̨GC�߳�
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: hoitFsGCThread
** ��������: HoitFS GC�߳�
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
**          uiThreshold GC��ֵ���ο�F2FS
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
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
        if(uiCurUsedSize > uiThreshold){                                    /* ִ��Foreground */
            if(bIsBackgroundThreadStart){
                API_MsgQueueSend(pfs->HOITFS_GCMsgQ, MSG_BG_GC_END, sizeof(MSG_BG_GC_END));
                API_ThreadJoin(hGcThreadId, LW_NULL);
                bIsBackgroundThreadStart = LW_FALSE;
            }
            hoitGCForgroudForce(pfs);
        }
        else {
            if(!bIsBackgroundThreadStart 
                && uiCurUsedSize > (pfs->HOITFS_totalSize / 2)){            /* ִ��Background */
                
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
