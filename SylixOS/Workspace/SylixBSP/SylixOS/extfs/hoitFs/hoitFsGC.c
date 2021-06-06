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

//TODO: GCʱ����һ����Ҫһ���տ��֧�֣���Ϊ����ĳ��������������ʵ�嶼���ڣ���Ҫ�޸��߼�
//TODO: move_home���ش���ֵ����ʾ�����ˣ�GC�׳��쳣

#define IS_MSG_GC_END(acMsg, stLen)             lib_memcmp(acMsg, MSG_GC_END, stLen) == 0
#define IS_MSG_BG_GC_END(acMsg, stLen)          lib_memcmp(acMsg, MSG_BG_GC_END, stLen) == 0
#define KILL_LOOP()                             break
/*********************************************************************************************************
** ��������: __hoitGCSectorRawInfoFixUp
** ��������: �ͷ�����pErasableSector�еĹ���RawInfo���޸�next_phys��ϵ
** �䡡��  : pErasableSector        Ŀ�������
** �䡡��  : �Ƿ�����գ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoitGCSectorRawInfoFixUp(PHOIT_ERASABLE_SECTOR pErasableSector){
    PHOIT_RAW_INFO          pRawInfoFirst;
    PHOIT_RAW_INFO          pRawInfoLast;
    
    PHOIT_RAW_INFO          pRawInfoTrailing;               /* ǰһ��ָ�� */
    PHOIT_RAW_INFO          pRawInfoTraverse;               /* ��ǰָ�� */
    PHOIT_RAW_INFO          pRawInfoObselete;

    BOOL                    bIsReset;

    pRawInfoFirst       = LW_NULL;
    pRawInfoLast        = LW_NULL;

    pRawInfoTrailing    = LW_NULL;
    pRawInfoTraverse    = pErasableSector->HOITS_pRawInfoFirst;
    
    bIsReset = LW_FALSE;

    if((pErasableSector->HOITS_pRawInfoCurGC 
    && pErasableSector->HOITS_pRawInfoCurGC->is_obsolete == HOIT_FLAG_OBSOLETE) 
    || pErasableSector->HOITS_pRawInfoCurGC == LW_NULL){                /* �����ǰGC RawInfo���ڣ��򻹲����ڵ�ǰGC RawInfo�������¿�ʼRawInfo��GC */
        bIsReset = LW_TRUE;
    }
    
    while (pRawInfoTraverse && pRawInfoTraverse->is_obsolete == HOIT_FLAG_OBSOLETE)           /* Ѱ�ҵ�һ����obselete��RawInfo�����ͷ��ѹ��ڵ�RawInfo */
    {
        pRawInfoObselete = pRawInfoTraverse;
        pRawInfoTraverse = pRawInfoTraverse->next_phys;
        if(pRawInfoObselete == pErasableSector->HOITS_pRawInfoLast){    /* ȫ�ǿգ�ֱ�ӷ��ؿ� */
            lib_free(pRawInfoObselete);
            pErasableSector->HOITS_pRawInfoFirst    = LW_NULL;
            pErasableSector->HOITS_pRawInfoLast     = LW_NULL;
            pErasableSector->HOITS_pRawInfoCurGC    = LW_NULL;
            pErasableSector->HOITS_pRawInfoPrevGC   = LW_NULL;
            pErasableSector->HOITS_pRawInfoLastGC   = LW_NULL;
            return LW_FALSE;
        }
        lib_free(pRawInfoObselete);
    }

    pRawInfoFirst    = pRawInfoLast = pRawInfoTraverse;                 /* ����RawInfo First��RawInfo Last */
    pRawInfoTrailing = pRawInfoTraverse;                    
    pRawInfoTraverse = pRawInfoTraverse->next_phys;
    
    while (LW_TRUE)
    {
        if(pRawInfoTrailing == pErasableSector->HOITS_pRawInfoLast
        || pRawInfoTraverse == LW_NULL){    /* ɨ����� */
            break;
        }
        if(pRawInfoTraverse->is_obsolete == HOIT_FLAG_OBSOLETE){                                      /* ������� */
            pRawInfoObselete                    = pRawInfoTraverse;             
            pRawInfoTrailing->next_phys         = pRawInfoTraverse->next_phys;  /* �޸�ָ�롪��ǰһ��ָ��ǰ�����һ�� */
            pRawInfoTraverse                    = pRawInfoTraverse->next_phys;  /* �õ�ǰ��Ϊ��һ�� */
            
            //!������GC�������޸�һ��Sector�ĸ���Size���������ֺ����ص����
            // pErasableSector->HOITS_uiUsedSize   -= pRawInfoObselete->totlen; 
            // pErasableSector->HOITS_uiFreeSize   += pRawInfoObselete->totlen; 
            
            lib_free(pRawInfoObselete);                                         /* �ͷŹ��ڵĿ� */
        }
        else {
            pRawInfoLast                = pRawInfoTraverse;                     /* ����pRawInfoLast */
            pRawInfoTrailing            = pRawInfoTraverse;              
            pRawInfoTraverse            = pRawInfoTraverse->next_phys;
        }
    }

    pErasableSector->HOITS_pRawInfoFirst    = pRawInfoFirst;
    pErasableSector->HOITS_pRawInfoLast     = pRawInfoLast;

    if(bIsReset){                                                   /* ��ʼ�� Last GC�� Info CurGC, Info Prev GC */
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
            pErasableSector->HOITS_pRawInfoLastGC = LW_NULL;        /* ��Sector��û��Raw Info�ˡ��� */
        }
    }
    return LW_TRUE;
}
/*********************************************************************************************************
** ��������: __hoitGCFindErasableSector
** ��������: ����HoitFS�豸ͷ�е���Ϣ��Ѱ��һ���ɲ������ݿ飬ĿǰѰ��FreeSize��С����Ϊ��GC
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_ERASABLE_SECTOR __hoitGCFindErasableSector(PHOIT_VOLUME pfs, ENUM_HOIT_GC_LEVEL gcLevel){
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
        if(hoitLogCheckIfLog(pfs, pErasableListTraverse)){          /* ������Log Sector�е��κ��ļ� */
            pErasableListTraverse   = pErasableListTraverse->HOITS_next;
            continue;
        }
        uiFreeSize  = pErasableListTraverse->HOITS_uiFreeSize;   
#ifdef GC_TEST                                  
        if(pErasableListTraverse->HOITS_next == LW_NULL){           /* ������һ��Sector�� */
            pErasableListTraverse->HOITS_uiFreeSize -= 3;           /* ����һ�� */
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
** ��������: __hoitGCCollectRawInfoAlive
** ��������: ��pErasableSector��pRawInfoCurGC���𣬻�ȡ��Ч��Ϣ��һ�λ�ȡһ��
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
** �䡡��  : ��ɶԸ�Sector��GC  LW_TRUE�� ����LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoitGCCollectRawInfoAlive(PHOIT_VOLUME pfs, PHOIT_ERASABLE_SECTOR pErasableSector, PHOIT_RAW_INFO pRawInfoCurGC){
    PHOIT_ERASABLE_SECTOR   pNowSectorOrigin;
    BOOL                    bIsMoveSuccess;

    if(pErasableSector == LW_NULL){
#ifdef GC_DEBUG
        printf("[%s] setcor can not be null\n", __func__);
#endif // GC_DEBUG
        return LW_TRUE;
    }
    
    pNowSectorOrigin = pfs->HOITFS_now_sector;              /* ����ԭ��now_sector */
    //!��RawInfo�����Ӧ������ʵ����
    bIsMoveSuccess = __hoit_move_home(pfs, pRawInfoCurGC);  /* ���ʧ�ܣ�˵����RawInfoҪô��LOGҪô��һ�δ�������ݣ�����ֱ������ */
    pfs->HOITFS_now_sector = pNowSectorOrigin;              /* �ָ�ԭ��now_sector */

    return bIsMoveSuccess;
}

/*********************************************************************************************************
** ��������: __hoitGCCollectSectorAlive
** ��������: ��pErasableSector��pRawInfoCurGC���𣬻�ȡ��Ч��Ϣ��һ�λ�ȡһ�������޸���Ӧָ��
** �䡡��  : pfs                HoitFS�ļ��豸ͷ
**          pErasableSector     
** �䡡��  : ��ɶԸ�Sector��GC  LW_TRUE�� ����LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoitGCCollectSectorAlive(PHOIT_VOLUME pfs, PHOIT_ERASABLE_SECTOR pErasableSector){
    BOOL                    bIsCollectOver;
    BOOL                    bIsMoveSuccess;
    BOOL                    bIsNeedMoreCollect;
    INTREG                  iregInterLevel;

    PHOIT_RAW_INFO          pRawInfoNextGC;
    PHOIT_RAW_INFO          pRawInfoCurGC;
    PHOIT_RAW_INFO          pRawInfoPrevGC;

    bIsNeedMoreCollect = __hoitGCSectorRawInfoFixUp(pErasableSector);            /* FixUp�󣬻���� HOITS_pRawInfoCurGC��HOITS_pRawInfoPrevGC��HOITS_pRawInfoLastGC�� */
    if(!bIsNeedMoreCollect){
        bIsCollectOver = LW_TRUE;
        goto __hoitGCCollectSectorAliveEnd;                                      /* ������ */
    }
#ifdef GC_DEBUG
    printf("[%s]: Fix over the Sector %d\n", __func__, pErasableSector->HOITS_bno);
#endif //GC_DEBUG
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

    if(pErasableSector->HOITS_pRawInfoLastGC == LW_NULL){                                  /* ������Last�� */
        bIsCollectOver = LW_TRUE;
    }

    if(pErasableSector->HOITS_pRawInfoLastGC 
    && pErasableSector->HOITS_pRawInfoLastGC->phys_addr == pRawInfoCurGC->phys_addr
    && pErasableSector->HOITS_pRawInfoLastGC->next_logic == pRawInfoCurGC->next_logic
    && pErasableSector->HOITS_pRawInfoLastGC->totlen == pRawInfoCurGC->totlen){             /* ���ܱȽ�next_phys��phys���ᱻ�޸�֮ */
        bIsCollectOver = LW_TRUE;
    }

    bIsMoveSuccess = __hoitGCCollectRawInfoAlive(pfs, pErasableSector, pRawInfoCurGC);


    if(bIsMoveSuccess){                                                  /* �ƶ��ɹ� */
        if(pRawInfoCurGC == pErasableSector->HOITS_pRawInfoFirst){       /* �����ǰGC����Sector�ĵ�һ��RawInfo */
            pErasableSector->HOITS_pRawInfoPrevGC = LW_NULL;             /* Prev = LW_NULL */
            pErasableSector->HOITS_pRawInfoFirst  = pRawInfoNextGC;      /* ����Sector�ĵ�һ��RawInfo */
        }
        else {                                                           /* ������ǵ�һ��RawInfo */
            pRawInfoPrevGC->next_phys = pRawInfoNextGC;                  /* ��ǰһ��RawInfoָ��Cur����һ�� */
            if(bIsCollectOver){                                          /* ���Cur�����һ��RawInfo */
                if(pfs->HOITFS_now_sector == pErasableSector){           /* �����RawInfo��Ȼ��д����Sector */
                    /* Do Nothing */
                }
                else {                                                   /* �����RawInfo��д�����Sector */
                    pRawInfoPrevGC->next_phys = LW_NULL;                 /* ��Lastָ��ָ��ǰһ��RawInfo���� */
                    pErasableSector->HOITS_pRawInfoLast = pRawInfoPrevGC;
                }
            }
        }
    }
    else {                                                               /* ���û��MOVE�ɹ� */
        pErasableSector->HOITS_pRawInfoPrevGC = pRawInfoCurGC;           /* Prev���ǵ�ǰ��RawInfo */
    }

    pErasableSector->HOITS_pRawInfoCurGC  = pRawInfoNextGC;              /* ����Curָ�� */

__hoitGCCollectSectorAliveEnd:    
    if(bIsCollectOver){
        pErasableSector->HOITS_pRawInfoCurGC  = LW_NULL;                  /* ��ǰSector��GC��RawInfoΪ�� */
        pErasableSector->HOITS_pRawInfoPrevGC = LW_NULL;
#ifdef GC_DEBUG
        API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
        printf("[%s] Sector %d is collected Over, Total Moved %dKB to Survivor Sector %d\n", 
                __func__, pErasableSector->HOITS_bno, (pfs->HOITFS_curGCSuvivorSector->HOITS_uiUsedSize / 1024), 
                pfs->HOITFS_curGCSuvivorSector->HOITS_bno);
        API_TShellColorEnd(STD_OUT);
#endif
    }

    return bIsCollectOver;
}

/*********************************************************************************************************
** ��������: __hoitGCClearBackground
** ��������: HoitFS�����̨GC�߳�
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __hoitGCClearBackground(PHOIT_VOLUME pfs, BOOL * pBIsBackgroundThreadStart, LW_OBJECT_HANDLE hGcThreadId){
    if(*pBIsBackgroundThreadStart){
        API_MsgQueueSend(pfs->HOITFS_GCMsgQ, MSG_BG_GC_END, sizeof(MSG_BG_GC_END));
        API_ThreadWakeup(hGcThreadId);
        API_ThreadJoin(hGcThreadId, LW_NULL);
        *pBIsBackgroundThreadStart = LW_FALSE;
    }
}
/*********************************************************************************************************
** ��������: hoitFSGCForgroudForce
** ��������: HoitFSǰ̨ǿ��GC��Greedy�㷨������������������Sector
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID hoitGCForegroundForce(PHOIT_VOLUME pfs){
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    INTREG                  iregInterLevel;
    BOOL                    bIsCollectOver;
    
    pErasableSector = LW_NULL;
    
    if(pfs->HOITFS_curGCSector == LW_NULL && pfs->HOITFS_curGCSuvivorSector == LW_NULL) {
        pErasableSector                 = __hoitGCFindErasableSector(pfs, GC_FOREGROUND);      
        pfs->HOITFS_curGCSector         = pErasableSector;     
        pfs->HOITFS_curGCSuvivorSector  = hoitFindAvailableSector(pfs);
    }

    if(pErasableSector){
        LW_SPIN_LOCK_QUICK(&pErasableSector->HOITS_lock, &iregInterLevel);      /* ���Լ����������������� */
    }

    while (LW_TRUE)
    {
        if(pErasableSector) {
            bIsCollectOver = __hoitGCCollectSectorAlive(pfs, pErasableSector);
            if(bIsCollectOver){                                                  /*! ��ʾ�ÿ� */
                pfs->HOITFS_curGCSector        = LW_NULL;                        /* ��ǰGC��SectorΪ�� */
                pfs->HOITFS_curGCSuvivorSector = LW_NULL;                        /* �Ҵ���SectorΪ�� */
                hoitResetSectorState(pfs->HOITFS_cacheHdr, pErasableSector);     /* ���ø�Sector״̬������Ϊ�� */
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

    if(pErasableSector){
        LW_SPIN_UNLOCK_QUICK(&pErasableSector->HOITS_lock, iregInterLevel);              /* ���Խ��� */
    }
}

/*********************************************************************************************************
** ��������: hoitFsGCBackgroundThread
** ��������: HoitFS��̨GC�̣߳�ֻ��������һ��ʵ��
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
                            LW_OPTION_NOT_WAIT);
        if(IS_MSG_BG_GC_END(acMsg, stLen)){
            KILL_LOOP();
        }

        if(pfs->HOITFS_curGCSector == LW_NULL && pfs->HOITFS_curGCSuvivorSector == LW_NULL) {
            pErasableSector                 = __hoitGCFindErasableSector(pfs, GC_BACKGROUND);
            pfs->HOITFS_curGCSector         = pErasableSector;     
            pfs->HOITFS_curGCSuvivorSector  = hoitFindAvailableSector(pfs);
        }

        if(pErasableSector) {
            LW_SPIN_LOCK_QUICK(&pErasableSector->HOITS_lock, &iregInterLevel); 
            bIsCollectOver = __hoitGCCollectSectorAlive(pfs, pErasableSector);      /* �������յ�һʵ�� */
            if(bIsCollectOver){
                pfs->HOITFS_curGCSector        = LW_NULL;                           /* ��ǰGC��SectorΪ�� */
                pfs->HOITFS_curGCSuvivorSector = LW_NULL;                           /* �Ҵ���SectorΪ�� */
                hoitResetSectorState(pfs->HOITFS_cacheHdr, pErasableSector);        /* ���ø�Sector״̬������Ϊ�� */
            }
            LW_SPIN_UNLOCK_QUICK(&pErasableSector->HOITS_lock, iregInterLevel);
        }
        else {
#ifdef GC_DEBUG
            API_ThreadLock();
            printf("[%s]: there's no sector can be GCed\n", __func__);
            API_ThreadUnlock();
#endif // GC_DEBUG
        }
        sleep(5);
    }
}


/*********************************************************************************************************
** ��������: hoitFsGCThread
** ��������: HoitFS GC�����̣߳����ڼ����ռ�仯���Ӷ��жϴ�ʱ��ִ�е�GC��ʽ
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
    CHAR                        acMsg[MAX_MSG_BYTE_SIZE];
    size_t                      stLen;

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
        API_MsgQueueReceive(pfs->HOITFS_GCMsgQ, 
                            acMsg, 
                            sizeof(acMsg), 
                            &stLen, 
                            10);
        if(IS_MSG_GC_END(acMsg, stLen)){                                            /* �ر�GC�����߳�  */
            printf("[%s] recev msg %s\n", __func__, MSG_GC_END);                
            __hoitGCClearBackground(pfs, &bIsBackgroundThreadStart, hGcThreadId);   /* �رպ�̨GC�߳� */
            KILL_LOOP();
        }
        
        uiCurUsedSize = pfs->HOITFS_totalUsedSize;
        if(uiCurUsedSize > uiThreshold){                                            /* ִ��Foreground */
            //__hoitGCClearBackground(pfs, &bIsBackgroundThreadStart, hGcThreadId); /* ����û��Ҫ��ֹGC��̨�̣߳���Ϊ�����������ƣ��Ǻ� */
            hoitGCForegroundForce(pfs);
        }
        else {
            if(!bIsBackgroundThreadStart 
                && uiCurUsedSize > (pfs->HOITFS_totalSize / 2)){                    /* ִ��Background */
                
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
        sleep(5);
    }

}

/*********************************************************************************************************
** ��������: hoitGCClose
** ��������: �ر� HoitFS GC�߳�
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
**          uiThreshold GC��ֵ���ο�F2FS
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID hoitGCClose(PHOIT_VOLUME pfs){
    if(pfs->HOITFS_hGCThreadId){
        API_MsgQueueSend(pfs->HOITFS_GCMsgQ, MSG_GC_END, sizeof(MSG_GC_END));
        API_MsgQueueShow(pfs->HOITFS_GCMsgQ);
        API_ThreadWakeup(pfs->HOITFS_hGCThreadId);                              /* ǿ�ƻ���GC�߳� */
        API_ThreadJoin(pfs->HOITFS_hGCThreadId, LW_NULL);
        API_MsgQueueDelete(&pfs->HOITFS_GCMsgQ);
        pfs->HOITFS_hGCThreadId = LW_NULL;
    }
    printf("================ Goodbye GC ================\n");
}

