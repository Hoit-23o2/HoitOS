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


typedef enum hoitGCLevel
{
    GC_FOREGROUND,
    GC_BACKGROUND
} ENUM_HOIT_GC_LEVEL;


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


    pRawInfoTrailing    = LW_NULL;
    pRawInfoTraverse    = pErasableSector->HOITS_pRawInfoFirst;
    
    while (!pRawInfoTraverse->is_obsolete)                  /* Ѱ�ҵ�һ����obselete��RawInfo�����ͷ��ѹ��ڵ�RawInfo */
    {
        pRawInfoObselete = pRawInfoTraverse;
        pRawInfoTraverse = pRawInfoTraverse->next_phys;
        lib_free(pRawInfoObselete);
    }

    pRawInfoFirst    = pRawInfoTraverse;                    /* ����RawInfo First */
    pRawInfoTrailing = pRawInfoTraverse;                    
    pRawInfoTraverse = pRawInfoTraverse->next_phys;
    
    while (LW_TRUE)
    {
        if(pRawInfoTrailing == pErasableSector->HOITS_pRawInfoLast){    /* ɨ����� */
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
    PHOIT_ERASABLE_SECTOR       pErasableList;
    PHOIT_ERASABLE_SECTOR       pErasableListTraverse;
    
    UINT                        uiMinVictimSan;            /* ��С�ܺ���Sanֵ */
    UINT                        uiVictimSan;               /* �ܺ���Sanֵ��ԽСԽ�ܺ� */

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
    PHOIT_RAW_INFO          pRawInfoLast;
    
    PCHAR                   pContent;
    
    PHOIT_RAW_HEADER        pRawHeader;

    pRawInfoCurGC   = pErasableSector->HOITS_pRawInfoCurGC;
    pRawInfoLast    = pErasableSector->HOITS_pRawInfoLast;
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
** ��������: hoitFsGCBackgroundThread
** ��������: HoitFS��̨GC�߳�
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: hoitFsGCBackgroundThread
** ��������: HoitFS��̨GC�߳�
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID hoitFsGCThread(PHOIT_VOLUME pfs){

}