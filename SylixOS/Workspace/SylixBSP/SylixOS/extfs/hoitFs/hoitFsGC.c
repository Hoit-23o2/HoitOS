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
/*********************************************************************************************************
** ��������: __hoitFsGCFindErasableSector
** ��������: ����HoitFS�豸ͷ�е���Ϣ��Ѱ��һ���ɲ������ݿ飬ĿǰѰ��FreeSize��С����Ϊ��GC
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __hoitFSGCCollectSetcorAlive
** ��������: ��pErasableSector��pRawInfoCurGC���𣬻�ȡ��Ч��Ϣ��һ�λ�ȡһ��
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
** �䡡��  : ��ɶԸ�Sector��GC  LW_TRUE�� ����LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
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

    //TODO:���������
    pContent = (PCHAR)lib_malloc(pRawInfoCurGC->totlen);
    hoitReadFromCache(pRawInfoCurGC->phys_addr, pContent, pRawInfoCurGC->totlen);
    pRawHeader = (PHOIT_RAW_HEADER)pContent;
    
    uiCurSectorNo   = pfs->HOITFS_now_sector->HOITS_bno;
    uiCurSectorOfs  = pfs->HOITFS_now_sector->HOITS_offset;
    
    if(!__HOIT_IS_OBSOLETE(pRawHeader)){
        if(__HOIT_IS_TYPE_DIRENT(pRawHeader)){
            pRawDirent = (PHOIT_RAW_INODE)pContent;
            //TODO:Cache��д��ʱ������now_sector
            
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
** ��������: hoitFSGCForgroudForce
** ��������: HoitFSǰ̨ǿ��GC
** �䡡��  : pfs        HoitFS�ļ��豸ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID hoitFSGCForgroudForce(PHOIT_VOLUME pfs){

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