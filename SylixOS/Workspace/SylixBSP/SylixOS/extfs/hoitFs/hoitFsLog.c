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
** ��   ��   ��: hoitFsLog.c
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 04 �� 25 ��
**
** ��        ��: ��־��ʵ��
*********************************************************************************************************/
#include "hoitFsLog.h"
#include "hoitFsLib.h"
#include "hoitFsCache.h"
/*********************************************************************************************************
** ��������: __hoitInsertLogSector
** ��������: ��pLogSectorList����һ���µ�LogSector�ڵ�
** �䡡��  : pfs            HoitFS�豸ͷ
**          pLogSectorNew   �µĽڵ�
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __hoitInsertLogSectorList(PHOIT_VOLUME pfs, PHOIT_LOG_SECTOR pLogSectorNew){
    PHOIT_LOG_SECTOR    pLogSectorTraverse;
    PHOIT_LOG_SECTOR    pLogSectorTrailling;

    if(pfs->HOITFS_erasableSectorList == LW_NULL){
        pfs->HOITFS_erasableSectorList = pLogSectorNew;
        return;
    }

    pLogSectorTraverse = pfs->HOITFS_logInfo->pLogSectorList;

    while (pLogSectorTraverse)
    {
        pLogSectorTrailling = pLogSectorTraverse;
        pLogSectorTraverse = pLogSectorTraverse->pErasableNextLogSector;
    }
    pLogSectorTrailling->pErasableNextLogSector = pLogSectorNew;
}
/*********************************************************************************************************
** ��������: __hoitDeleteLogSectorList
** ��������: ��pLogSectorListɾ��һ����LogSector�ڵ�
** �䡡��  : pfs            HoitFS�豸ͷ
**          pLogSector      ��ɾ���ڵ�
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __hoitDeleteLogSectorList(PHOIT_VOLUME pfs, PHOIT_LOG_SECTOR pLogSector){
    PHOIT_LOG_SECTOR    pLogSectorTraverse;
    PHOIT_LOG_SECTOR    pLogSectorTrailling;

    if(pfs->HOITFS_logInfo->pLogSectorList == pLogSector){
        pfs->HOITFS_logInfo->pLogSectorList = pLogSector->pErasableNextLogSector;
        lib_free(pLogSector);
        return;
    }

    pLogSectorTraverse = pfs->HOITFS_logInfo->pLogSectorList;
    

    while (pLogSectorTraverse)
    {
        pLogSectorTrailling = pLogSectorTraverse;
        pLogSectorTraverse = pLogSectorTraverse->pErasableNextLogSector;
        
        if(pLogSectorTraverse == pLogSector){
            pLogSectorTrailling->pErasableNextLogSector = pLogSector->pErasableNextLogSector;
            lib_free(pLogSector);
            break;
        }
    }
}

/*********************************************************************************************************
** ��������: __hoitLogSectorCleanUp
** ��������: ���LOG SectorΪ����
** �䡡��  : pfs            HoitFS�豸ͷ
**           pLogSector     ����ǵ�Setcor
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __hoitLogSectorCleanUp(PHOIT_VOLUME pfs, PHOIT_LOG_SECTOR pLogSector){
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    PHOIT_RAW_INFO          pRawInfoTraverse;

    pErasableSector  =  &pLogSector->ErasableSetcor;
    pRawInfoTraverse =  pErasableSector->HOITS_pRawInfoFirst;

    while (LW_TRUE)
    {
        __hoit_del_raw_data(pfs, pRawInfoTraverse);
        pRawInfoTraverse->is_obsolete = 1;
        
        if(pRawInfoTraverse == pErasableSector->HOITS_pRawInfoLast){
            break;
        }
        pRawInfoTraverse = pRawInfoTraverse->next_phys;
    }

    __hoitDeleteLogSectorList(pfs, pLogSector);
}

/*********************************************************************************************************
** ��������: __hoitLogHdrCleanUp
** ��������: ���LOG HdrΪ����
** �䡡��  : pfs            HoitFS�豸ͷ
**           pLogInfo     LOG �ļ�ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __hoitLogHdrCleanUp(PHOIT_VOLUME pfs, PHOIT_LOG_INFO pLogInfo){
    UINT                    uiRawLogHdrAddr;
    UINT                    uiSectorNum;
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    
    PHOIT_RAW_INFO          pRawInfoTraverse;

    
    uiRawLogHdrAddr = pLogInfo->uiRawLogHdrAddr;
    
    uiSectorNum     = hoitGetSectorNo(uiRawLogHdrAddr);
    pErasableSector = hoitFindSector(pfs->HOITFS_cacheHdr, uiSectorNum);

    pRawInfoTraverse = pErasableSector->HOITS_pRawInfoFirst;
    while (pRawInfoTraverse)
    {
        if(pRawInfoTraverse->phys_addr == uiRawLogHdrAddr){
            __hoit_del_raw_data(pfs, pRawInfoTraverse);
            pRawInfoTraverse->is_obsolete = 1;
            return;
        }
        pRawInfoTraverse = pRawInfoTraverse->next_phys;
    }
#ifdef DEBUG_LOG
    printf("[%s] LOG Hdr not find\n", __func__);
#endif // DEBUG_LOG
}
/*********************************************************************************************************
** ��������: __hoitScanLogSector
** ��������: ɨ��Log Sector����¼�����Ϣ
** �䡡��  : pfs            HoitFS�豸ͷ
**          pRawLog         Flash�ϵ�pRawLog����ʵ��
**          puiEntityNum    ��¼ʵ������
** �䡡��  : дƫ��
** ȫ�ֱ���:
** ����ģ��:
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
            
            /* ����ʼ��pRawLog��Ӧ��RawInfo���뵽 LOG SECTOR �� */
            pRawInfo                = (PHOIT_RAW_INFO)lib_malloc(sizeof(HOIT_RAW_INFO));
            pRawInfo->phys_addr     = pcCurSectorPos;
            pRawInfo->totlen        = pRawLog->totlen;
            pRawInfo->is_obsolete   = 0;
            pRawInfo->next_logic    = LW_NULL;
            pRawInfo->next_phys     = LW_NULL;
            __hoit_add_raw_info_to_sector(&pfs->HOITFS_logInfo->pLogSectorList->ErasableSetcor, 
                                          pRawInfo); 
            
            pcCurSectorPos += __HOIT_MIN_4_TIMES(pRawHeader->totlen);
            uiOfs = (pcCurSectorPos - pcLogSector); 
            *puiEntityNum = *puiEntityNum + 1;
        }
        else {
            pcCurSectorPos += 4;   /* ÿ���ƶ�4�ֽ� */
        }
    }
    lib_free(pcLogSector);
    return uiOfs;
}
/*********************************************************************************************************
** ��������: __hoitFindAvailableSector
** ��������: Ѱ��һ��ȫ�յ�Sector
** �䡡��  : pfs            HoitFS�豸ͷ
** �䡡��  : һ��ȫ�յ�Sector
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_ERASABLE_SECTOR __hoitFindAvailableSector(PHOIT_VOLUME pfs){
    UINT                    uiAvaiSectorNum;
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    
    uiAvaiSectorNum = hoitFindNextToWrite(pfs->HOITFS_cacheHdr, HOIT_CACHE_TYPE_DATA_EMPTY, LW_NULL);
    if(uiAvaiSectorNum == PX_ERROR){
        return LW_NULL;
    }
#ifdef DEBUG_LOG
    printf("[%s] sector %d will be our log sector \n", __func__ ,uiAvaiSectorNum);
#endif // DEBUG_LOG
    pErasableSector = hoitFindSector(pfs->HOITFS_cacheHdr, uiAvaiSectorNum);
    return pErasableSector;
}
/*********************************************************************************************************
** ��������: hoitLogInit
** ��������: Build��ʱ�����û��ɨ�赽LOG���͵���InitLOG����ʼ��һ����־ϵͳ
** �䡡��  : pfs            HoitFS�豸ͷ
**          uiLogSize       Log��С
**          uiSectorNum     Log Sector��������ʱֻ֧��1
** �䡡��  : LOG�ļ���Ϣ
** ȫ�ֱ���:
** ����ģ��:
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
#ifdef DEBUG_LOG
        printf("[%s] only support one log sector for now \n", __func__);
#endif // DEBUG_LOG
        return LW_NULL;
    }
    
    pRawLog            = (PHOIT_RAW_LOG)lib_malloc(sizeof(HOIT_RAW_LOG));
    lib_memset(pRawLog, 0, sizeof(HOIT_RAW_LOG));
    pRawLog->file_type = S_IFLOG;
    pRawLog->magic_num = HOIT_MAGIC_NUM;
    pRawLog->flag      = HOIT_FLAG_TYPE_LOG | HOIT_FLAG_OBSOLETE;
    pRawLog->ino       = __hoit_alloc_ino(pfs);
    pRawLog->totlen    = sizeof(HOIT_RAW_LOG);
    pRawLog->version   = pfs->HOITFS_highest_version++;

    //TODO: �ҵ�һ���յ�Sector��ΪLOG Sector
    pErasableSector    = __hoitFindAvailableSector(pfs);
    if(pErasableSector == LW_NULL){
#ifdef DEBUG_LOG
        printf("[%s] can't find a sector for log\n", __func__);
#endif // DEBUG_LOG
        pfs->HOITFS_logInfo = LW_NULL;
        return LW_NULL;
    }
    pRawLog->uiLogFirstAddr = pErasableSector->HOITS_addr;
    pRawLog->uiLogSize = uiLogSize;

    uiSectorAddr = hoitWriteToCache(pfs->HOITFS_cacheHdr, (PCHAR)pRawLog, pRawLog->totlen);
    hoitFlushCache(pfs->HOITFS_cacheHdr);
    
    printf("[%s] our LOG HDR' version is %d, inode no is %d\n", 
            __func__, pRawLog->version, pRawLog->ino);

    /* ����ʼ��pRawLog��Ӧ��RawInfo���뵽������ */
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
    pLogInfo->uiRawLogHdrAddr          = uiSectorAddr;
    
    if(pfs->HOITFS_logInfo != LW_NULL){
        lib_free(pfs->HOITFS_logInfo);
        pfs->HOITFS_logInfo = LW_NULL;
    }

    pfs->HOITFS_logInfo                = pLogInfo;
    return pLogInfo;
}

/*********************************************************************************************************
** ��������: hoitLogOpen
** ��������: Build��ʱ�����ɨ�赽 ��Ч LOG���͵���hoitLogOpen���Ӷ���ʼ��LOGϵͳ
** �䡡��  : pfs            HoitFS�豸ͷ
**          pRawLog        Flash�ϵ�RAW_LOG��Ϣ 
** �䡡��  : LOG�ļ���Ϣ
** ȫ�ֱ���:
** ����ģ��:
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
#ifdef DEBUG_LOG
        printf("[%s] not found log setcor\n", __func__);
#endif // DEBUG_LOG
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
** ��������: hoitLogRead
** ��������: ��ȡLog Sector�ϵ���Ϣ
** �䡡��  : pfs            HoitFS�豸ͷ
**          uiEntityNum      ��Sector�ϵ�uiEntityNum��ʵ�����Ϣ
** �䡡��  :����RAW Headerָ���ַ��������LOG Header
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PCHAR hoitLogEntityGet(PHOIT_VOLUME pfs, UINT uiEntityNum){
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    PHOIT_RAW_INFO          pRawInfoTraverse;

    UINT                    uiEntitySize;
    UINT                    uiEntityIndex;
    PCHAR                   pcEntity;
    
    if(uiEntityNum > pfs->HOITFS_logInfo->uiLogEntityCnt){
#ifdef DEBUG_LOG
        printf("[%s] uiEntityNum start from 0, Log has no enough entities to get\n",__func__);
#endif // DEBUG_LOG
        return PX_ERROR;
    }

    uiEntityIndex = 0;
    pErasableSector = &pfs->HOITFS_logInfo->pLogSectorList->ErasableSetcor;     /* ��ȡLog Sector */
    pRawInfoTraverse = pErasableSector->HOITS_pRawInfoFirst;

    while (LW_TRUE)
    {
        if(uiEntityIndex == uiEntityNum){
            break;
        }
        uiEntityIndex++;
        pRawInfoTraverse = pRawInfoTraverse->next_phys;
    }
    
    uiEntitySize = pRawInfoTraverse->totlen - sizeof(HOIT_RAW_LOG);
    pcEntity = (PCHAR)lib_malloc(uiEntitySize);

    hoitReadFromCache(pfs->HOITFS_cacheHdr, pRawInfoTraverse->phys_addr + sizeof(HOIT_RAW_LOG),     /* Ŀ�� Entity ����RawInfo */
                      pcEntity, uiEntitySize);  
    
    return pcEntity;
}


/*********************************************************************************************************
** ��������: hoitLogAppend
** ��������: ��LOG Sector׷��д��LOG
** �䡡��  : pfs                HoitFS�豸ͷ
**          pcEntityContent     Ҫд��Log��ʵ��
**          uiEntitySize        ʵ���С 
** �䡡��  : ׷�Ӵ�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT hoitLogAppend(PHOIT_VOLUME pfs, PCHAR pcEntityContent, UINT uiEntitySize){
    PHOIT_RAW_HEADER    pRawHeader;
    PHOIT_RAW_LOG       pRawLog;
    PHOIT_RAW_INFO      pRawInfo;

    PHOIT_LOG_INFO      pLogInfo;
    UINT                uiLogAddr;
    UINT                uiLogCurOfs;
    UINT                uiLogSize;
    UINT                uiLogRemainSize;
    
    UINT                uiSize;
    PCHAR               pcLogContent;

    if(pfs->HOITFS_logInfo == LW_NULL){
#ifdef DEBUG_LOG
        printf("[%s] no space for log sector\n", __func__);
#endif // DEBUG_LOG
        return (LOG_APPEND_NO_SECTOR);
    }

    if(uiEntitySize < sizeof(HOIT_RAW_HEADER)){
#ifdef DEBUG_LOG
        printf("[%s] cannot append something that not is Entity\n", __func__);
#endif // DEBUG_LOG
        return (LOG_APPEND_BAD_ENTITY);
    }

    pRawHeader = (PHOIT_RAW_HEADER)pcEntityContent;
    if(pRawHeader->magic_num != HOIT_MAGIC_NUM){
#ifdef DEBUG_LOG
        printf("[%s] cannot append something that not is Entity\n", __func__);
#endif // DEBUG_LOG
        return (LOG_APPEND_BAD_ENTITY);
    }


    pLogInfo                = pfs->HOITFS_logInfo;
    uiLogAddr               = pLogInfo->uiLogCurAddr;
    uiLogSize               = pLogInfo->uiLogSize;
    uiLogCurOfs             = pLogInfo->uiLogCurOfs;
    uiLogRemainSize         = uiLogSize - uiLogCurOfs;
    uiSize                  = uiEntitySize + sizeof(HOIT_RAW_LOG);
                                                            /* ����һ��LOGʵ��ͷ */                                                                        
    pRawLog                 = (PHOIT_RAW_LOG)lib_malloc(sizeof(HOIT_RAW_LOG));
    pRawLog->file_type      = S_IFLOG;
    pRawLog->flag           = HOIT_FLAG_TYPE_LOG | HOIT_FLAG_OBSOLETE;
    pRawLog->magic_num      = HOIT_MAGIC_NUM;
    pRawLog->totlen         = uiSize;
    pRawLog->uiLogFirstAddr = PX_ERROR;
    pRawLog->uiLogSize      = uiLogSize;
    pRawLog->version        = pfs->HOITFS_highest_version++;
    pRawLog->ino            = __hoit_alloc_ino(pfs);


    pcLogContent            = (PCHAR)lib_malloc(uiSize);
    lib_memcpy(pcLogContent, pRawLog, sizeof(HOIT_RAW_LOG));
    lib_memcpy(pcLogContent + sizeof(HOIT_RAW_LOG), pcEntityContent, uiEntitySize);

    pRawInfo                = (PHOIT_RAW_INFO)lib_malloc(sizeof(HOIT_RAW_INFO));    /* ����RawInfo */
    pRawInfo->is_obsolete   = 0;
    pRawInfo->next_logic    = LW_NULL;
    pRawInfo->next_phys     = LW_NULL;
    pRawInfo->totlen        = uiSize;

    if(uiLogRemainSize < uiSize){                                           /* ��ǰLog Sector����д���������ǰSector */
        __hoitLogSectorCleanUp(pfs, pfs->HOITFS_logInfo->pLogSectorList);   /* ���LOG HDRָ���Sector�����ݣ�ȫ�����Ϊ���� */
        __hoitLogHdrCleanUp(pfs, pLogInfo);                                 /* ���LOG HDR�������Ϊ���� */
        if(hoitLogInit(pfs, uiLogSize, 1) == LW_NULL) {                     /* �´�һ��LOG */
#ifdef DEBUG_LOG
            printf("[%s] log memory not enough\n", __func__);
#endif // DEBUG_LOG
            return (LOG_APPEND_NO_SECTOR);
        }

        uiLogAddr   = pfs->HOITFS_logInfo->uiLogCurAddr;                 
        uiLogCurOfs = pfs->HOITFS_logInfo->uiLogCurOfs;
    }

    pRawInfo->phys_addr     = uiLogAddr + uiLogCurOfs;
    
    __hoit_add_raw_info_to_sector(&pfs->HOITFS_logInfo->pLogSectorList->ErasableSetcor, pRawInfo);      /* ����д��LOG���뵽Sector���� */
    pRawHeader = (PHOIT_RAW_HEADER)pcLogContent;

    hoitWriteThroughCache(pfs->HOITFS_cacheHdr, uiLogAddr + uiLogCurOfs, pcLogContent, uiSize);
    hoitFlushCache(pfs->HOITFS_cacheHdr);
                                                                    /* �޸���ӦLOG���� */
    pfs->HOITFS_logInfo->uiLogEntityCnt++;
    pfs->HOITFS_logInfo->uiLogCurOfs += uiSize;

    lib_free(pcLogContent);

    return (LOG_APPEND_OK);
}
/*********************************************************************************************************
** ��������: hoitLogCheckIfLog
** ��������: �鿴ĳ��Sector�Ƿ���Log Sector
** �䡡��  : pfs                HoitFS�豸ͷ
**          pErasableSector     �����Ŀ�
** �䡡��  : �� LW_TRUE�� �� LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL hoitLogCheckIfLog(PHOIT_VOLUME pfs, PHOIT_ERASABLE_SECTOR pErasableSector){
    PHOIT_LOG_SECTOR    pLogSectorTraverse;
    if(pfs->HOITFS_logInfo == LW_NULL){
        return LW_FALSE;
    }
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
