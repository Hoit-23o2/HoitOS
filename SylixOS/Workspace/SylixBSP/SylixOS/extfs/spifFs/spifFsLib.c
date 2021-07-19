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
** ��   ��   ��: spifFsLib.c
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 06 �� 01��
**
** ��        ��: Spiffs�ļ�ϵͳ����
*********************************************************************************************************/
#include "spifFsLib.h"
#include "spifFsCache.h"
#include "spifFsGC.h"
#include "spifFsFDLib.h"

INT32 spiffsPhysCpy(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler, UINT32 uiDst,
                    UINT32 uiSrc, UINT32 uiLen) {
    (void)fileHandler;
    INT32 iRes;
    UCHAR ucBuffer[SPIFFS_COPY_BUFFER_STACK];
    while (uiLen > 0) {
        UINT32 uiChunkSize = MIN(SPIFFS_COPY_BUFFER_STACK, uiLen);
        iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_MOVS, fileHandler, 
                               uiSrc, uiChunkSize, ucBuffer);
        SPIFFS_CHECK_RES(iRes);
        iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_MOVD,  fileHandler, 
                                uiDst, uiChunkSize, ucBuffer);
        SPIFFS_CHECK_RES(iRes);
        uiLen -= uiChunkSize;
        uiSrc += uiChunkSize;
        uiDst += uiChunkSize;
    }
    return SPIFFS_OK;
}
/*********************************************************************************************************
** ��������: __spiffsObjLookUpFindIdAndSpanVistor
** ��������: Ѱ��ID�������Ӧ��Span
** �䡡��  : pfs          �ļ�ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffsObjLookUpFindIdAndSpanVistor(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, SPIFFS_BLOCK_IX blkIX, INT iLookUpEntryIX, 
                                           const PVOID pUserConst, PVOID pUserVar){
    INT32 iRes;
    SPIFFS_PAGE_HEADER pageHeader;
    SPIFFS_PAGE_IX pageIX = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iLookUpEntryIX);
    iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_READ, 0, 
                           SPIFFS_PAGE_TO_PADDR(pfs, pageIX), 
                           sizeof(SPIFFS_PAGE_HEADER), (PCHAR)&pageHeader);
    SPIFFS_CHECK_RES(iRes);
    //TODO:ʲô��˼������
    if(pageHeader.objId == objId &&
       pageHeader.spanIX == *((SPIFFS_SPAN_IX *)pUserVar) &&
       (pageHeader.flags & (SPIFFS_PH_FLAG_FINAL | SPIFFS_PH_FLAG_DELET | SPIFFS_PH_FLAG_USED)) == SPIFFS_PH_FLAG_DELET &&
       !((objId & SPIFFS_OBJ_ID_IX_FLAG) &&                                             /* ����Obj IX */
       (pageHeader.flags & SPIFFS_PH_FLAG_IXDELE) == 0 && pageHeader.spanIX == 0) &&    /* ��û�б�ɾ���Ҳ��ǵ�0��Span */
       (pUserConst == LW_NULL || *((const SPIFFS_PAGE_IX *)pUserConst) != pageIX)){     
        return SPIFFS_OK;
    }
    else {
        return SPIFFS_VIS_COUNTINUE;
    }
}
/*********************************************************************************************************
** ��������: __spiffsObjLookUpScanVistor
** ��������: ɨ�貢ͳ��Page��Ϣ��Vistor
** �䡡��  : pfs          �ļ�ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffsObjLookUpScanVistor(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, SPIFFS_BLOCK_IX blkIX, INT iLookUpEntryIX, 
                                  const PVOID pUserConst, PVOID pUserVar){
    (VOID) blkIX;
    (VOID) pUserConst;
    (VOID) pUserVar;

    if(objId == SPIFFS_OBJ_ID_FREE){
        if(iLookUpEntryIX == 0){        /* һ�������objIdΪ�գ�Ĭ�ϸÿ�Ϊ�գ������⣿��ʵ�Ϻܼ򵥣�LFSʹȻ*/
            pfs->uiFreeBlks++;
        }
    }
    else if(objId == SPIFFS_OBJ_ID_DELETED){
        pfs->uiStatsPageDeleted++;
    }
    else {
        pfs->uiStatsPageAllocated++;
    }

    return SPIFFS_VIS_COUNTINUE;
}
/*********************************************************************************************************
** ��������: __spiffsObjLookUpFindFreeObjIdBitmapVistor
** ��������: ��λ��WorkBuffer���ڴ��ռ���Ч����Ϣ
** �䡡��  : pfs          �ļ�ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffsObjLookUpFindFreeObjIdBitmapVistor(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, SPIFFS_BLOCK_IX blkIX, INT iLookUpEntryIX, 
                                                 const PVOID pUserConst, PVOID pUserVar){
    SPIFFS_OBJ_ID objIdMin; 
    PCHAR pucConflictingName;
    UINT32 bitIX;
    UINT32 byteIX;
    SPIFFS_PAGE_IX pageIX;
    SPIFFS_PAGE_OBJECT_IX_HEADER objIXHdr;
    INT32 iRes = SPIFFS_OK;

    if(objId != SPIFFS_OBJ_ID_FREE && objId != SPIFFS_OBJ_ID_DELETED){
       objIdMin = *((SPIFFS_OBJ_ID*)pUserVar);
        pucConflictingName = (const PCHAR)pUserConst;
        /* �ҵ�һ�����ΪIndex��Entry */
        if(pucConflictingName && (objId & SPIFFS_OBJ_ID_IX_FLAG)){
            pageIX = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iLookUpEntryIX);
            iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_READ, 0, 
                                   SPIFFS_PAGE_TO_PADDR(pfs, pageIX), 
                                   sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER), &objIXHdr);
            SPIFFS_CHECK_RES(iRes);
            if(objIXHdr.pageHdr.spanIX == 0 &&
               (objIXHdr.pageHdr.flags & (SPIFFS_PH_FLAG_DELET | SPIFFS_PH_FLAG_FINAL | SPIFFS_PH_FLAG_IXDELE)) 
               == (SPIFFS_PH_FLAG_DELET | SPIFFS_PH_FLAG_IXDELE)){
                if(lib_strcmp((const PCHAR)pUserConst, (PCHAR)objIXHdr.ucName) == 0){       /* �ļ�����ͻ */
                    return SPIFFS_ERR_CONFLICTING_NAME;
                }
            }
        }

        objId   &= ~SPIFFS_OBJ_ID_IX_FLAG;
        bitIX   = (objId - objIdMin) & 0x111;
        byteIX  = (objId - objIdMin) >> 3;
        if(byteIX >= 0 && byteIX < SPIFFS_CFG_LOGIC_PAGE_SZ(pfs)){
            pfs->pucWorkBuffer[byteIX] |= (1 << bitIX);
        }
    }
    return SPIFFS_VIS_COUNTINUE;
}
/*********************************************************************************************************
** ��������: __spiffsObjLookUpFindFreeObjIdCompactVistor
** ��������: ��λ��WorkBuffer���ڴ��ռ���Ч����Ϣ������ѹ���ռ�
** �䡡��  : pfs          �ļ�ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffsObjLookUpFindFreeObjIdCompactVistor(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, SPIFFS_BLOCK_IX blkIX, INT iLookUpEntryIX, 
                                                  const PVOID pUserConst, PVOID pUserVar){
    (VOID) pUserVar;
    INT32 iRes = SPIFFS_OK;
    const PSPIFFS_FREE_OBJ_ID_STATE pState = (const PSPIFFS_FREE_OBJ_ID_STATE)pUserConst;
    SPIFFS_PAGE_OBJECT_IX_HEADER objIXHdr;
    SPIFFS_PAGE_IX pageIX;
    PCHAR pucObjBitMap;
    UINT   uiIX;

    if(objId != SPIFFS_OBJ_ID_FREE && objId != SPIFFS_OBJ_ID_DELETED 
       && (objId & SPIFFS_OBJ_ID_IX_FLAG)){
        pageIX = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iLookUpEntryIX);
        iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_READ, 0, 
                               SPIFFS_PAGE_TO_PADDR(pfs, pageIX), 
                               sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER), &objIXHdr);
        SPIFFS_CHECK_RES(iRes);
        if(objIXHdr.pageHdr.spanIX == 0 &&
           (objIXHdr.pageHdr.flags & (SPIFFS_PH_FLAG_INDEX | SPIFFS_PH_FLAG_FINAL | SPIFFS_PH_FLAG_DELET))
           == (SPIFFS_PH_FLAG_DELET)){
            if(pState->pucConflictingName &&
               lib_strcmp((const PCHAR)pUserConst, (PCHAR)objIXHdr.ucName) == 0){       /* �ļ�����ͻ */
                return SPIFFS_ERR_CONFLICTING_NAME;
            }

            objId &= ~SPIFFS_OBJ_ID_IX_FLAG;
            if(objId >= pState->objIdMin && objId <= pState->objIdMax){
                pucObjBitMap = (PCHAR)(pfs->pucWorkBuffer);
                /* (id - min) * page_sz  / (max - min)*/
                uiIX = (objId - pState->objIdMin) / pState->uiCompaction;   /* �ص��ĵڼ���ҳ */
                SPIFFS_DBG("free_obj_id: add ix "_SPIPRIi" for id "_SPIPRIid" min"_SPIPRIid" max"_SPIPRIid" comp:"_SPIPRIi"\n", 
                           uiIX, objId, pState->objIdMin, pState->objIdMax, pState->uiCompaction);
                pucObjBitMap[uiIX]++;
            }
        }
    }
    return SPIFFS_VIS_COUNTINUE;
}

INT32 __spiffsObjectFindObjectIndexHeaderByNameVisitor(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, SPIFFS_BLOCK_IX blkIX, INT iLookUpEntryIX, 
                                                       const PVOID pUserConst, PVOID pUserVar){
    (VOID)pUserVar;
    INT32 iRes;
    SPIFFS_PAGE_OBJECT_IX_HEADER objIXHdr;
    SPIFFS_PAGE_IX pageIX = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iLookUpEntryIX);
    if (objId == SPIFFS_OBJ_ID_FREE || objId == SPIFFS_OBJ_ID_DELETED ||
        (objId & SPIFFS_OBJ_ID_IX_FLAG) == 0) {
        return SPIFFS_VIS_COUNTINUE;
    }
    iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_READ, 0, 
                           SPIFFS_PAGE_TO_PADDR(pfs, pageIX), sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)
                           ,(PCHAR)&objIXHdr);
    SPIFFS_CHECK_RES(iRes);
    if (objIXHdr.pageHdr.spanIX == 0 &&
        (objIXHdr.pageHdr.flags & (SPIFFS_PH_FLAG_DELET | SPIFFS_PH_FLAG_FINAL | SPIFFS_PH_FLAG_IXDELE)) ==
            (SPIFFS_PH_FLAG_DELET | SPIFFS_PH_FLAG_IXDELE)) {
        if (strcmp((const PCHAR)pUserConst, (PCHAR)objIXHdr.ucName) == 0) {
            return SPIFFS_OK;
        }
    }

    return SPIFFS_VIS_COUNTINUE;
}
INT32 __spiffsReadDirVisitor(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, SPIFFS_BLOCK_IX blkIX, INT iLookUpEntryIX, 
                             const PVOID pUserConst, PVOID pUserVar) {
    (VOID)pUserConst;
    INT32 iRes;
    SPIFFS_PAGE_OBJECT_IX_HEADER objIXHdr;
    if (objId == SPIFFS_OBJ_ID_FREE || objId == SPIFFS_OBJ_ID_DELETED ||
        (objId & SPIFFS_OBJ_ID_IX_FLAG) == 0) {
        return SPIFFS_VIS_COUNTINUE;
    }

    SPIFFS_PAGE_IX pageIX = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iLookUpEntryIX);
    iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_READ, 0, 
                            SPIFFS_PAGE_TO_PADDR(pfs, pageIX), 
                            sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER), (PCHAR)&objIXHdr);
    if (iRes != SPIFFS_OK) 
        return iRes;
    if ((objId & SPIFFS_OBJ_ID_IX_FLAG) &&
        objIXHdr.pageHdr.spanIX == 0 &&
        (objIXHdr.pageHdr.flags & (SPIFFS_PH_FLAG_DELET | SPIFFS_PH_FLAG_FINAL | SPIFFS_PH_FLAG_IXDELE)) ==
        (SPIFFS_PH_FLAG_DELET | SPIFFS_PH_FLAG_IXDELE)) {
        PSPIFFS_DIRENT pDirent = (PSPIFFS_DIRENT)pUserVar;
        pDirent->objId = objId;
        lib_strcpy((PCHAR)pDirent->ucName, (PCHAR)objIXHdr.ucName);
        pDirent->objType = objIXHdr.type;
        pDirent->uiSize = objIXHdr.uiSize == SPIFFS_UNDEFINED_LEN ? 0 : objIXHdr.uiSize;
        pDirent->pageIX = pageIX;
        return SPIFFS_OK;
    }
    return SPIFFS_VIS_COUNTINUE;
}

/*********************************************************************************************************
** ��������: spiffsObjLookUpFindEntryVisitor
** ��������: ��Vistor������ÿ��Entry�����ʵ�objIdʱ�ᷢ���ص���flagsҲ��ص��й�
** �䡡��  : pfs          �ļ�ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjLookUpFindEntryVisitor(PSPIFFS_VOLUME pfs, SPIFFS_BLOCK_IX blkIXStarting, INT iLookUpEntryStarting,
                                      UINT8 flags, SPIFFS_OBJ_ID objId, spiffsVisitorFunc vistor,
                                      const PVOID pUserConst, PVOID pUserVar, SPIFFS_BLOCK_IX *pBlkIX, INT *piLookUpEntry){
    INT32           iRes               = SPIFFS_OK;
    INT32           iEntryCount        = pfs->uiBlkCount * SPIFFS_OBJ_LOOKUP_MAX_ENTRIES(pfs);  /* �ܹ�����Entry */
    SPIFFS_BLOCK_IX blkIXCur           = blkIXStarting;
    UINT32          uiBlkCurAddr       = SPIFFS_BLOCK_TO_PADDR(pfs, blkIXCur);
    SPIFFS_OBJ_ID   *pObjLookUpBuffer  = (SPIFFS_OBJ_ID *)pfs->pucLookupWorkBuffer;
    INT             iEntryCur          = iLookUpEntryStarting;
    INT             iEntriesPerPage    = SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) / sizeof(SPIFFS_OBJ_ID); /* LookUp Pageһ��Page�����ж��ٸ�Entry */
    
    SPIFFS_PAGE_IX  pageIXOffset;   /* ���Blk LookUpҳ�棬1 ~ SPIFFS_OBJ_LOOKUP_PAGES */
    INT             iEntryOffset;              

    /* ������һ��������Entry�� - 1���Թ�Erase Count�������һ���� */
    if(iEntryCur > (INT)SPIFFS_OBJ_LOOKUP_MAX_ENTRIES(pfs) - 1){
        iEntryCur = 0;
        blkIXCur++;
        uiBlkCurAddr = SPIFFS_BLOCK_TO_PADDR(pfs, blkIXCur);
        if(blkIXCur >= pfs->uiBlkCount){
            if(flags & SPIFFS_VIS_NO_WRAP){
                return SPIFFS_VIS_END;
            }
            else {
                iEntryCur = 0;
                blkIXCur = 0;
            }
        }
    }

    /* ����ÿ��Blk */
    while (iRes == SPIFFS_OK && iEntryCount > 0)    
    {
        pageIXOffset = iEntryCur / iEntriesPerPage;  
        /* �鿴ÿ��LookUp Page */
        while (iRes == SPIFFS_OK && pageIXOffset < (INT)SPIFFS_OBJ_LOOKUP_PAGES(pfs))
        {
            iEntryOffset = pageIXOffset * iEntriesPerPage;
            iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU | SPIFFS_OP_C_READ, 0, 
                                   uiBlkCurAddr + pageIXOffset * SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), 
                                   SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), pfs->pucLookupWorkBuffer);

            /* �鿴LookUp Page�ϵ�����Entry */
            while (iRes == SPIFFS_OK && 
                   iEntryCur < (INT)SPIFFS_OBJ_LOOKUP_MAX_ENTRIES(pfs) &&   /* ���һ��ҳ�� */
                   iEntryCur - iEntryOffset < iEntriesPerPage)              /* �����һ��ҳ�� */
            {
                if((flags & SPIFFS_VIS_CHECK_ID) == 0 || 
                   pObjLookUpBuffer[iEntryCur - iEntryOffset] == objId){
                    if(pBlkIX){
                        *pBlkIX = blkIXCur;
                    } 
                    if(piLookUpEntry){
                        *piLookUpEntry = iEntryCur;
                    }
                    if(vistor){
                        iRes = vistor(pfs, 
                                      (flags & SPIFFS_VIS_CHECK_PH) ? objId : pObjLookUpBuffer[iEntryCur - iEntryOffset],
                                      blkIXCur,
                                      iEntryCur, 
                                      pUserConst,
                                      pUserVar);
                        if(iRes == SPIFFS_VIS_COUNTINUE || iRes == SPIFFS_VIS_COUNTINUE_RELOAD) {
                            if(iRes == SPIFFS_VIS_COUNTINUE_RELOAD){
                                iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU | SPIFFS_OP_C_READ, 0, 
                                                       uiBlkCurAddr + pageIXOffset * SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), 
                                                       SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), pfs->pucLookupWorkBuffer);
                                SPIFFS_CHECK_RES(iRes);
                            }
                            iRes = SPIFFS_OK;
                            iEntryCur++;
                            iEntryCount--;
                            continue;
                        }
                        else {
                            return iRes;
                        }
                    }
                    else {
                        //TODO: Ϊɶ���Ƿ��� SPIFFS_VIS_END
                        return SPIFFS_OK;
                    }
                }
                iEntryCount--;
                iEntryCur++;
            } /* ��������ÿ��Look Up Page��Entry */
            pageIXOffset++;
        } /* ��������ÿ��Look Up Page */
        iEntryCur = 0;
        blkIXCur++;
        uiBlkCurAddr = SPIFFS_BLOCK_TO_PADDR(pfs, blkIXCur);
        if(blkIXCur >= pfs->uiBlkCount){
            if(flags & SPIFFS_VIS_NO_WRAP){
                return SPIFFS_VIS_END;
            }
            else {
                iEntryCur = 0;
                blkIXCur = 0;
            }
        }
    } /* ��������ÿ��Blk */
    
    SPIFFS_CHECK_RES(iRes);
    return SPIFFS_VIS_END;
}
/*********************************************************************************************************
** ��������: spiffsObjLookUpScan
** ��������: ɨ��Flash�����ϵ�����Lookup Page����¼ҳ��״̬��Deleted��Used�ȣ������ҵ������������
** �䡡��  : pfs          �ļ�ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjLookUpScan(PSPIFFS_VOLUME pfs){
    INT32           iRes;
    SPIFFS_BLOCK_IX blkIX;
    INT             iEntry;
    SPIFFS_BLOCK_IX blkIXUnerased = (SPIFFS_BLOCK_IX)-1;
    SPIFFS_OBJ_ID   objIdEraseCount;
    SPIFFS_OBJ_ID   objIdEraseCountFinal;
    SPIFFS_OBJ_ID   objIdEraseCountMin = SPIFFS_OBJ_ID_FREE;
    SPIFFS_OBJ_ID   objIdEraseCountMax = 0;
    SPIFFS_OBJ_ID   objIdMagic;

    blkIX           = 0;
    
    /* ���EraseCount�ļ��� */
    while (blkIX < pfs->uiBlkCount)
    {
#ifdef SPIFFS_USE_MAGIC
        iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU | SPIFFS_OP_C_READ, 0, 
                               SPIFFS_MAGIC_PADDR(pfs, blkIX), sizeof(SPIFFS_OBJ_ID), 
                               (PCHAR)&objIdMagic);
        SPIFFS_CHECK_RES(iRes);
        //TODO: ����ɨ���ʱ����Щ��δ���ã�Ҳ����˵����Ҫ��format����ʹ�ø��ļ�ϵͳ�
        if(objIdMagic != SPIFFS_MAGIC_PADDR(pfs, blkIX)){
            if(blkIXUnerased == SPIFFS_OBJ_ID_FREE){   /* ����һ��δ��д�飬�п����ǵ����ˣ�Magic�Ų�ͬ */
                blkIXUnerased = blkIX;
            }
            else {
                SPIFFS_CHECK_RES(SPIFFS_ERR_NOT_A_FS);
            }
        }
#endif
        iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_READ, 0, 
                               SPIFFS_ERASE_COUNT_PADDR(pfs, blkIX), sizeof(SPIFFS_OBJ_ID),
                               (PCHAR)&objIdEraseCount);
        SPIFFS_CHECK_RES(iRes);
        if(objIdEraseCount != SPIFFS_OBJ_ID_FREE){      
            objIdEraseCountMin = MIN(objIdEraseCountMin, objIdEraseCount);
            objIdEraseCountMax = MAX(objIdEraseCountMax, objIdEraseCount);
        }
        blkIX++;
    }

    //TODO: ��������������������μ����
    /* δ��������ϵͳ */
    if(objIdEraseCountMin == 0 && objIdEraseCountMax == SPIFFS_OBJ_ID_FREE){
        objIdEraseCountFinal = 0;
    }
    //TODO: ���λΪ1����Index Page������������Լ����
    /* Max: 11 -  Min: 1 = 3 - 1 > (11 >> 2) = 1 */
    else if((objIdEraseCountMax - objIdEraseCountMin) > (SPIFFS_OBJ_ID_FREE) / 2){
        objIdEraseCountFinal = objIdEraseCountMin + 1;
    }
    else {
        objIdEraseCountFinal = objIdEraseCountMax + 1;
    }

    pfs->uiMaxEraseCount = objIdEraseCountFinal;

    if(blkIXUnerased != SPIFFS_OBJ_ID_FREE) {
        SPIFFS_DBG("mount: erase block "_SPIPRIbl"\n", blkIXUnerased);
        iRes = spiffsEraseBlk(pfs, blkIXUnerased);
        SPIFFS_CHECK_RES(iRes);
    }

    iRes = spiffsObjLookUpFindEntryVisitor(pfs, 0, 0, 0, 0, __spiffsObjLookUpScanVistor, 
                                           LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    if(iRes == SPIFFS_VIS_END){
        iRes = SPIFFS_OK;
    }

    SPIFFS_CHECK_RES(iRes);
    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsObjLookUpFindFreeObjId
** ��������: ����Ѱ��Free Obj ID
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjLookUpFindFreeObjId(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID *pObjId, const PCHAR pucConflictingName){
    INT32                    iRes         = SPIFFS_OK;
    UINT32                   uiMaxObjects = (pfs->uiBlkCount * SPIFFS_OBJ_LOOKUP_MAX_ENTRIES(pfs)) / 2;  /* ���Objects������Ϊһ��Object������һ��Index��һ��Data���� */
    SPIFFS_OBJ_ID            objIdFree    = SPIFFS_OBJ_ID_FREE;
    SPIFFS_FREE_OBJ_ID_STATE state;
    UINT                     i,j;
    
    UINT32                   uiMinIndex = 0;
    UINT8                    uiMinCount = (UINT8)-1;
    PCHAR                   pucBitMap;

    UINT8                    uiMask8;
    //TODO: State����������ģ�
    //!���ڴ��
    state.objIdMin = 1;
    state.objIdMax = uiMaxObjects + 1;
    if(state.objIdMax & SPIFFS_OBJ_ID_IX_FLAG){
        state.objIdMax = ((SPIFFS_OBJ_ID)-1) & ~SPIFFS_OBJ_ID_IX_FLAG;
    }
    state.uiCompaction = 0;
    state.pucConflictingName = pucConflictingName;

    while (iRes == SPIFFS_OK && objIdFree == SPIFFS_OBJ_ID_FREE)
    {   
        /* ����װ��һ��ҳ���һ���ֽ�8λ */
        if(state.objIdMax - state.objIdMin 
           <= (SPIFFS_OBJ_ID)SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) * 8){
            /* ���Է���һ��bitmap�� */
            SPIFFS_DBG("free_obj_id: BITMap min:"_SPIPRIid" max:"_SPIPRIid"\n", state.objIdMin, state.objIdMax);
            lib_memset(pfs->pucWorkBuffer, 0, SPIFFS_CFG_LOGIC_PAGE_SZ(pfs));   /* ���work�����ڼ�¼λͼ���� */
            iRes = spiffsObjLookUpFindEntryVisitor(pfs, 0, 0, 0, 0, __spiffsObjLookUpFindFreeObjIdBitmapVistor,
                                                   pucConflictingName, &state.objIdMin, LW_NULL, LW_NULL);
            if(iRes == SPIFFS_VIS_END){
                iRes = SPIFFS_OK;
            }
            SPIFFS_CHECK_RES(iRes);

            /* ����ÿ���ֽ� */
            for (i = 0; i < SPIFFS_CFG_LOGIC_PAGE_SZ(pfs); i++)
            {
                uiMask8 = pfs->pucWorkBuffer[i];
                if(uiMask8 == (UINT8)-1){
                    continue;
                }
                /* ����ÿ��λ */
                /* 8λ���� */
                for (j = 0; j < 8; j++)
                {
                    if((uiMask8 & (1 << j)) == 0){
                        *pObjId = state.objIdMin + j + (i << 3);
                        return SPIFFS_OK;
                    }
                }
                
            }
            return SPIFFS_ERR_FULL;     /* û�п����Obj�� */           
        }
        /* ����װ��һ��ҳ����������Զ���ص��������С�ģ��㶮�� */
        else {
            if(state.uiCompaction != 0){
                uiMinIndex = 0;
                uiMinCount = (UINT8)-1;
                pucBitMap = pfs->pucWorkBuffer;

                for (i = 0; i < SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) / sizeof(UINT8); i++)
                {
                    if(pucBitMap[i] < uiMinCount){
                        uiMinCount = pucBitMap[i];
                        uiMinIndex = i;
                        if(uiMinCount == 0){
                            break;
                        }
                    }
                }

                if(uiMinCount == state.uiCompaction){       /* û�п���OBJ�� */
                    SPIFFS_DBG("free_obj_id: compacted table is full\n");
                    return SPIFFS_ERR_FULL;
                }
                
                SPIFFS_DBG("free_obj_id: COMP select index:"_SPIPRIi" min_count:"_SPIPRIi" min:"_SPIPRIid" max:"_SPIPRIid" compact:"_SPIPRIi"\n",
                            uiMinIndex, uiMinCount, state.objIdMin, state.objIdMax, state.uiCompaction);
                if(uiMinCount == 0){
                    *pObjId = uiMinIndex * state.uiCompaction + state.objIdMin;
                    return SPIFFS_OK;
                }
                else {
                    /* ����ѹ����Χ */
                    SPIFFS_DBG("free_obj_id: COMP SEL chunk:"_SPIPRIi" min:"_SPIPRIid" -> "_SPIPRIid"\n", state.uiCompaction, state.objIdMin, state.objIdMin + uiMinIndex *  state.uiCompaction);
                    state.objIdMin += (uiMinIndex * state.uiCompaction);
                    state.objIdMax = state.objIdMin + state.uiCompaction;
                }

                if(state.objIdMax - state.objIdMin 
                    <= (SPIFFS_OBJ_ID)SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) * 8){
                    /* ������һҳװ�ˣ��Ǿ�ֱ��װ������ѹ���� */
                    continue;
                }
            }
            state.uiCompaction = (state.objIdMax - state.objIdMin) / ((SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) / sizeof(UINT8))); /* ҳ������ */
            SPIFFS_DBG("free_obj_id: COMP min:"_SPIPRIid" max:"_SPIPRIid" compact:"_SPIPRIi"\n", state.objIdMin, 
                       state.objIdMax, state.uiCompaction);
            lib_memset(pfs->pucWorkBuffer, 0, SPIFFS_CFG_LOGIC_PAGE_SZ(pfs));
            iRes = spiffsObjLookUpFindEntryVisitor(pfs, 0, 0, 0, 0, 
                                                   __spiffsObjLookUpFindFreeObjIdCompactVistor, 
                                                   &state, LW_NULL, LW_NULL, LW_NULL);
            if(iRes == SPIFFS_VIS_END){
                iRes = SPIFFS_OK;
            }
            SPIFFS_CHECK_RES(iRes);
            state.pucConflictingName = LW_NULL; /* ��һ�ξ͹��� */
        }
    }
}
/*********************************************************************************************************
** ��������: spiffsObjLookUpFindId
** ��������: ����Ѱ�Ҹ�����ObjId
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjLookUpFindId(PSPIFFS_VOLUME pfs, SPIFFS_BLOCK_IX blkIXStarting, INT iLookUpEntryStarting,
                            SPIFFS_OBJ_ID objId, SPIFFS_BLOCK_IX *pBlkIX, INT *piLookUpEntry){
    INT32 iRes = spiffsObjLookUpFindEntryVisitor(pfs, blkIXStarting, iLookUpEntryStarting, 
                                                SPIFFS_VIS_CHECK_ID, objId, LW_NULL, LW_NULL, LW_NULL, 
                                                pBlkIX, piLookUpEntry);
    if(iRes == SPIFFS_VIS_END) {
        iRes = SPIFFS_ERR_NOT_FOUND;
    }
    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsObjLookUpFindFreeEntry
** ��������: ����Ѱ��Free Look Up Entry
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjLookUpFindFreeEntry(PSPIFFS_VOLUME pfs, SPIFFS_BLOCK_IX blkIXStarting, INT iLookUpEntryStarting,
                                   SPIFFS_BLOCK_IX *pBlkIX, INT *piLookUpEntry){
    INT32 iRes;
    if (!pfs->uiCleaningFlag && pfs->uiFreeBlks < 2) {
        iRes = spiffsGCQuick(pfs, 0);
        if (iRes == SPIFFS_ERR_NO_DELETED_BLOCKS) {
            iRes = SPIFFS_OK;
        }
        SPIFFS_CHECK_RES(iRes);
        if (pfs->uiFreeBlks < 2) {
            return SPIFFS_ERR_FULL;
        }
    }
    /* �ҵ�һ��IdΪ111��Entry */
    iRes = spiffsObjLookUpFindId(pfs, blkIXStarting, iLookUpEntryStarting,
                                 SPIFFS_OBJ_ID_FREE, pBlkIX, piLookUpEntry);
    if (iRes == SPIFFS_OK) {
        pfs->blkIXFreeCursor = *pBlkIX;
        pfs->objLookupEntryFreeCursor = (*piLookUpEntry) + 1;
        if (*piLookUpEntry == 0) {
            pfs->uiFreeBlks--;
        }
    }
    if (iRes == SPIFFS_ERR_FULL) {
        SPIFFS_DBG("pfs full\n");
    }
    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsObjLookUpFindIdAndSpan
** ��������: ����Ѱ�Ҹ�����ObjID��SpanIX������PageIX
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None 
** ȫ�ֱ���: 
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjLookUpFindIdAndSpan(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, SPIFFS_SPAN_IX spanIX,
                                   SPIFFS_PAGE_IX pageIXExclusion, SPIFFS_PAGE_IX *pPageIX){
    INT32 iRes;
    SPIFFS_BLOCK_IX blkIX;
    INT iEntry;

    iRes = spiffsObjLookUpFindEntryVisitor(pfs, pfs->blkIXCursor, pfs->objLookupEntryCursor, SPIFFS_VIS_CHECK_ID, objId,
                                           __spiffsObjLookUpFindIdAndSpanVistor, pageIXExclusion ? &pageIXExclusion : LW_NULL,
                                           &spanIX, &blkIX, &iEntry);
    if (iRes == SPIFFS_VIS_END) {
        iRes = SPIFFS_ERR_NOT_FOUND;
    }

    SPIFFS_CHECK_RES(iRes);

    if (pPageIX) {
        *pPageIX = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iEntry);
    }

    pfs->blkIXCursor = blkIX;
    pfs->objLookupEntryCursor = iEntry;

    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsObjectFindObjectIndexHeaderByName
** ��������: ����Ѱ��Free Obj ID
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjectFindObjectIndexHeaderByName(PSPIFFS_VOLUME pfs, UCHAR ucName[SPIFFS_OBJ_NAME_LEN], SPIFFS_PAGE_IX *pPageIX){
    INT32 iRes;
    SPIFFS_BLOCK_IX blkIX;
    INT iEntry;

    iRes = spiffsObjLookUpFindEntryVisitor(pfs,
                                           pfs->blkIXCursor,
                                           pfs->objLookupEntryCursor,
                                           0,
                                           0,
                                           __spiffsObjectFindObjectIndexHeaderByNameVisitor,
                                           ucName,
                                           LW_NULL,
                                           &blkIX,
                                           &iEntry);
    
    if (iRes == SPIFFS_VIS_END) {
        iRes = SPIFFS_ERR_NOT_FOUND;
    }
    SPIFFS_CHECK_RES(iRes);

    if (pPageIX) {
        *pPageIX = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iEntry);
    }

    pfs->blkIXCursor = blkIX;
    pfs->objLookupEntryCursor = iEntry;

    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsPageIndexCheck
** ��������: ���ҳ������
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsPageIndexCheck(PSPIFFS_VOLUME pfs, PSPIFFS_FD pFd, 
                           SPIFFS_PAGE_IX pageIX, SPIFFS_SPAN_IX spanIX){
    UINT32 iRes = SPIFFS_OK;
    SPIFFS_PAGE_HEADER pageHeader;
    if (pageIX == (SPIFFS_PAGE_IX)-1) {
        // referring to page 0xffff...., bad object index
        return SPIFFS_ERR_INDEX_FREE;
    }
    if (pageIX % SPIFFS_PAGES_PER_BLOCK(pfs) < SPIFFS_OBJ_LOOKUP_PAGES(pfs)) {
        /* ҳ�治��ָ��LookUp page */
        // referring to an object lookup page, bad object index
        return SPIFFS_ERR_INDEX_LU;
    }
    if (pageIX > SPIFFS_MAX_PAGES(pfs)) {
        // referring to a bad page
        return SPIFFS_ERR_INDEX_INVALID;
    }

    /* Check Page */
    iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_READ, pFd->fileN,
                           SPIFFS_PAGE_TO_PADDR(pfs, pageIX), sizeof(SPIFFS_PAGE_HEADER), 
                           (PCHAR)&pageHeader);
    SPIFFS_CHECK_RES(iRes);
    SPIFFS_VALIDATE_OBJIX(pageHeader, pFd->objId, spanIX);
    
    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsPageDataCheck
** ��������: �������ҳ��
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsPageDataCheck(PSPIFFS_VOLUME pfs, PSPIFFS_FD pFd,  
                          SPIFFS_PAGE_IX pageIX, SPIFFS_SPAN_IX spanIX){
    UINT32 iRes = SPIFFS_OK;
    SPIFFS_PAGE_HEADER pageHeader;
    if (pageIX == (SPIFFS_PAGE_IX)-1) {
        // referring to page 0xffff...., bad object index
        return SPIFFS_ERR_INDEX_REF_FREE;
    }
    if (pageIX % SPIFFS_PAGES_PER_BLOCK(pfs) < SPIFFS_OBJ_LOOKUP_PAGES(pfs)) {
        // referring to an object lookup page, bad object index
        return SPIFFS_ERR_INDEX_REF_LU;
    }
    if (pageIX > SPIFFS_MAX_PAGES(pfs)) {
        // referring to a bad page
        return SPIFFS_ERR_INDEX_REF_INVALID;
    }
    /* Check Page */
    iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_READ, pFd->fileN, 
                           SPIFFS_PAGE_TO_PADDR(pfs, pageIX),
                           sizeof(SPIFFS_PAGE_HEADER),(PCHAR)&pageHeader);
    SPIFFS_CHECK_RES(iRes);
    /* ��֤����Indexҳ�� */
    SPIFFS_VALIDATE_DATA(pageHeader, pFd->objId & ~SPIFFS_OBJ_ID_IX_FLAG, spanIX);
    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsPageDelete
** ��������: ɾ��ָ��ҳ�棨���ɾ�����ɣ�
** �䡡��  : pfs          �ļ�ͷ
**           pageIX         ҳ��IX
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsPageDelete(PSPIFFS_VOLUME pfs, SPIFFS_PAGE_IX pageIX){
    INT32 iRes;
    // mark deleted iEntry in source object lookup
    /* �ȱ��LookUp Entry */
    SPIFFS_OBJ_ID d_obj_id = SPIFFS_OBJ_ID_DELETED;
    iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_LU | SPIFFS_OP_C_DELE, 0,
                            SPIFFS_BLOCK_TO_PADDR(pfs, SPIFFS_BLOCK_FOR_PAGE(pfs, pageIX)) + 
                            SPIFFS_OBJ_LOOKUP_ENTRY_FOR_PAGE(pfs, pageIX) * sizeof(SPIFFS_PAGE_IX),
                            sizeof(SPIFFS_OBJ_ID), (PCHAR)&d_obj_id);
    SPIFFS_CHECK_RES(iRes);

    pfs->uiStatsPageAllocated--;
    pfs->uiStatsPageDeleted++;


    // mark deleted in source page
    /* �ٱ�Ǵ�ɾ��ҳ�� */
    UINT8 flags = 0xff;
    flags &= ~(SPIFFS_PH_FLAG_DELET | SPIFFS_PH_FLAG_USED);
    iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_DELE, 0,
                            SPIFFS_PAGE_TO_PADDR(pfs, pageIX) + offsetof(SPIFFS_PAGE_HEADER, flags),
                            sizeof(flags), &flags);
    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsPageDelete
** ��������: ɾ��ָ��ҳ�棨���ɾ�����ɣ�
** �䡡��  : pfs          �ļ�ͷ
**           pageIX         ҳ��IX
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsPageAllocateData(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, PSPIFFS_PAGE_HEADER pPageHeader,
                             PCHAR pData, UINT32 uiLen, UINT32 uiPageOffs, BOOL bIsFinalize,
                             SPIFFS_PAGE_IX *pageIX) {
    INT32 iRes = SPIFFS_OK;
    SPIFFS_BLOCK_IX blkIX;
    INT iEntry;

    // find free iEntry
    /* �ҵ�һ�����е�Entry */
    iRes = spiffsObjLookUpFindFreeEntry(pfs, pfs->blkIXFreeCursor, 
                                        pfs->objLookupEntryFreeCursor, &blkIX, &iEntry);
    SPIFFS_CHECK_RES(iRes);

    // occupy page in object lookup
    /* �õ�ǰ����ҳ���objIdռ�����entry */
    iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_LU | SPIFFS_OP_C_UPDT, 0, 
                            SPIFFS_BLOCK_TO_PADDR(pfs, blkIX) + iEntry * sizeof(SPIFFS_OBJ_ID), 
                            sizeof(SPIFFS_OBJ_ID), (PCHAR)&objId);
    SPIFFS_CHECK_RES(iRes);

    pfs->uiStatsPageAllocated++;

    // write page header
    pPageHeader->flags &= ~SPIFFS_PH_FLAG_USED;
    iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_UPDT, 0, 
                            SPIFFS_OBJ_LOOKUP_ENTRY_TO_PADDR(pfs, blkIX, iEntry), 
                            sizeof(SPIFFS_PAGE_HEADER), (PCHAR)pPageHeader);
    SPIFFS_CHECK_RES(iRes);

    // write page data
    if (pData) {
        iRes = spiffsCacheWrite(pfs,  SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_UPDT, 0, 
                                SPIFFS_OBJ_LOOKUP_ENTRY_TO_PADDR(pfs, blkIX, iEntry) + sizeof(SPIFFS_PAGE_HEADER) + uiPageOffs, 
                                uiLen, pData);
        SPIFFS_CHECK_RES(iRes);
    }

    // finalize header if necessary
    /* ����д�� */
    if (bIsFinalize && (pPageHeader->flags & SPIFFS_PH_FLAG_FINAL)) {
        pPageHeader->flags &= ~SPIFFS_PH_FLAG_FINAL;
        iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_UPDT, 0, 
                                SPIFFS_OBJ_LOOKUP_ENTRY_TO_PADDR(pfs, blkIX, iEntry) + offsetof(SPIFFS_PAGE_HEADER, flags),  
                                sizeof(UINT8),(PCHAR)&pPageHeader->flags);
        SPIFFS_CHECK_RES(iRes);
    }

    // return uiByteHasWritten page
    /* ���ص�ǰд���ҳ�� */
    if (pageIX) {
        *pageIX = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iEntry);
    }

    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsObjectUpdateIndexHdr
** ��������: ��������ҳͷ
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjectUpdateIndexHdr(PSPIFFS_VOLUME pfs, PSPIFFS_FD pFd, SPIFFS_OBJ_ID objId, SPIFFS_PAGE_IX pageIXObjIXHdr,
                                 PCHAR pucNewObjIXHdrData , const UCHAR ucName[], UINT32 uiSize, SPIFFS_PAGE_IX *pageIXNew){
    INT32                           iRes = SPIFFS_OK;
    PSPIFFS_PAGE_OBJECT_IX_HEADER   objIXHdr;                           /* Indexҳ���SpanIX = 0��ͷ */
    SPIFFS_PAGE_IX                  pageIXObjIXHdrNew;                  /* SpanIX = 0��Indexҳ���page IX */

    /* ��ȡ��Ӧ����ҳ��ObjID */
    objId |=  SPIFFS_OBJ_ID_IX_FLAG;

    /* ��ȡIndexҳ���Hdr */
    if (pucNewObjIXHdrData) {
        // object index header page already given to us, no need to load it
        objIXHdr = (PSPIFFS_PAGE_OBJECT_IX_HEADER)pucNewObjIXHdrData;
    } 
    else {
        // read object index header page
        iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_READ, pFd->fileN, 
                               SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjIXHdr), SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), 
                               pfs->pucWorkBuffer);
        SPIFFS_CHECK_RES(iRes);
        objIXHdr = (PSPIFFS_PAGE_OBJECT_IX_HEADER)pfs->pucWorkBuffer;
    }

    SPIFFS_VALIDATE_OBJIX(objIXHdr->pageHdr, objId, 0);

    // change name
    if (ucName) {
        strncpy((char*)objIXHdr->ucName, (const char*)ucName, sizeof(objIXHdr->ucName) - 1);
        ((char*) objIXHdr->ucName)[sizeof(objIXHdr->ucName) - 1] = '\0';
    }

    if (uiSize) {
        objIXHdr->uiSize = uiSize;
    }
    
    // move and update page
    //TODO: ΪʲôҪ�ƶ�ҳ�棬��Ϊҳ�������
    iRes = spiffsPageMove(pfs, pFd == LW_NULL ? LW_NULL : pFd->fileN, 
                          (PCHAR)objIXHdr, objId, 0, pageIXObjIXHdr, &pageIXObjIXHdrNew);

    if (iRes == SPIFFS_OK) {
        if (pageIXNew) {
            *pageIXNew = pageIXObjIXHdrNew;
        }
        // callback on object index update
        spiffsCBObjectEvent(pfs, (PSPIFFS_PAGE_OBJECT_IX)objIXHdr, 
                            pucNewObjIXHdrData ? SPIFFS_EV_IX_UPD : SPIFFS_EV_IX_UPD_HDR,
                            objId, objIXHdr->pageHdr.spanIX, pageIXObjIXHdrNew, objIXHdr->uiSize);
        if (pFd){
            pFd->pageIXObjIXHdr = pageIXObjIXHdrNew; // if this is not in the registered cluster
        } 
    }

    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsObjectCreate
** ��������: ���ڴ���һ��Object
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjectCreate(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId,
                         const UCHAR ucPath[], 
                         SPIFFS_OBJ_TYPE type, SPIFFS_PAGE_IX* pPageIXObjIndexHdr)
{
    INT32 iRes = SPIFFS_OK;
    SPIFFS_BLOCK_IX blkIX;
    SPIFFS_PAGE_OBJECT_IX_HEADER  objIXHdr;
    INT iEntry;
    
    iRes = spiffsGCCheck(pfs, SPIFFS_DATA_PAGE_SIZE(pfs));
    SPIFFS_CHECK_RES(iRes);

    objId |= SPIFFS_OBJ_ID_IX_FLAG;             /* ת���ɸ�ID��Ӧ��Ŀ¼ */
    iRes = spiffsObjLookUpFindFreeEntry(pfs, pfs->blkIXFreeCursor, 
                                        pfs->objLookupEntryFreeCursor, &blkIX, &iEntry);
    SPIFFS_DBG("create: found free page @ "_SPIPRIpg" blkIX:"_SPIPRIbl" iEntry:"_SPIPRIsp"\n", 
               (SPIFFS_PAGE_IX)SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iEntry), blkIX, iEntry);

    // occupy page in object lookup
    iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_LU | SPIFFS_OP_C_UPDT, 0, 
                            SPIFFS_BLOCK_TO_PADDR(pfs, blkIX) + iEntry * sizeof(SPIFFS_OBJ_ID), 
                            sizeof(SPIFFS_OBJ_ID), (PCHAR)&objId);
#ifdef SPIFFS_CACHE_TEST
    objId = 0;
    iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU | SPIFFS_OP_C_READ, 0, 
                            SPIFFS_BLOCK_TO_PADDR(pfs, blkIX) + iEntry * sizeof(SPIFFS_OBJ_ID), 
                            sizeof(SPIFFS_OBJ_ID), (PCHAR)&objId);
#endif
    SPIFFS_CHECK_RES(iRes);

    pfs->uiStatsPageAllocated++;

    // write empty object index page
    objIXHdr.pageHdr.objId = objId;
    objIXHdr.pageHdr.spanIX = 0;
    objIXHdr.pageHdr.flags = 0xff & ~(SPIFFS_PH_FLAG_FINAL | SPIFFS_PH_FLAG_INDEX | SPIFFS_PH_FLAG_USED);
    objIXHdr.type = type;
    objIXHdr.uiSize = SPIFFS_UNDEFINED_LEN; // keep ones so we can update later without wasting this page
    strncpy((PCHAR)objIXHdr.ucName, ucPath, sizeof(objIXHdr.ucName) - 1);
    ((PCHAR)objIXHdr.ucName)[sizeof(objIXHdr.ucName) - 1] = '\0';

    // update page
    iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_UPDT, 0, 
                            SPIFFS_OBJ_LOOKUP_ENTRY_TO_PADDR(pfs, blkIX, iEntry), 
                            sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER), (PCHAR)&objIXHdr);

    //ANCHOR : �����������Բ���

    SPIFFS_CHECK_RES(iRes);
    spiffsCBObjectEvent(pfs, (SPIFFS_PAGE_OBJECT_IX *)&objIXHdr,
                        SPIFFS_EV_IX_NEW, objId, 0, 
                        SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iEntry), 
                        SPIFFS_UNDEFINED_LEN);

    if (pPageIXObjIndexHdr) {
        *pPageIXObjIndexHdr = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iEntry);
    }

    return iRes;
}

/*********************************************************************************************************
** ��������: spiffsObjectOpenByPage
** ��������: ����һ��pageIX��һ���ļ�
** �䡡��  : pfs          �ļ�ͷ
**           pageIX        ���ص�Object ID
**           pFd           ���е�Fd
**           flags         �򿪷�ʽ
**           mode          Ŀ��������Posix����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjectOpenByPage(PSPIFFS_VOLUME pfs, SPIFFS_PAGE_IX pageIX, 
                             PSPIFFS_FD pFd, SPIFFS_FLAGS flags, SPIFFS_MODE mode){
    //TODO: ��ҳ����Indexҳ��
    (VOID)mode;
    INT32                        iRes = SPIFFS_OK;
    SPIFFS_PAGE_OBJECT_IX_HEADER objIXHdr;
    SPIFFS_OBJ_ID                objId;
    SPIFFS_BLOCK_IX              blkIX = SPIFFS_BLOCK_FOR_PAGE(pfs, pageIX);
    INT                          iEntry = SPIFFS_OBJ_LOOKUP_ENTRY_FOR_PAGE(pfs, pageIX);

    iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_READ, pFd->fileN, 
                           SPIFFS_PAGE_TO_PADDR(pfs, pageIX), 
                           sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER), (PCHAR)&objIXHdr);
    SPIFFS_CHECK_RES(iRes);


    iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU | SPIFFS_OP_C_READ, 0, 
                           SPIFFS_BLOCK_TO_PADDR(pfs, blkIX) + iEntry * sizeof(SPIFFS_OBJ_ID), 
                           sizeof(SPIFFS_OBJ_ID), (PCHAR)&objId);

    pFd->pfs = pfs;
    pFd->pageIXObjIXHdr = pageIX;
    pFd->uiSize = objIXHdr.uiSize;
    pFd->uiOffset = 0;
    pFd->pageIXObjIXCursor = pageIX;
    pFd->spanIXObjIXCursor = 0;
    pFd->objId = objId;
    pFd->flags = flags;

    SPIFFS_VALIDATE_OBJIX(objIXHdr.pageHdr, pFd->objId, 0);

    SPIFFS_DBG("open: pFd "_SPIPRIfd" is obj id "_SPIPRIid"\n", 
                pFd->fileN, pFd->objId);

    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsObjectTruncate
** ��������: ���ڽض�һ��Object
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjectTruncate(PSPIFFS_FD pFd, UINT32 uiNewSize, BOOL bIsRemoveFull){
    INT32          iRes = SPIFFS_OK;
    PSPIFFS_VOLUME pfs = pFd->pfs;

    /* ���ø��κ����� */
    if ((pFd->uiSize == SPIFFS_UNDEFINED_LEN || pFd->uiSize == 0) && !bIsRemoveFull) {
        // no op
        return iRes;
    }

    // need 2 pages if not removing: object index page + possibly chopped data page
    if (bIsRemoveFull == LW_FALSE) {
        iRes = spiffsGCCheck(pfs, SPIFFS_DATA_PAGE_SIZE(pfs) * 2);
        SPIFFS_CHECK_RES(iRes);
    }

    SPIFFS_PAGE_IX                  pageIXObjIX = pFd->pageIXObjIXHdr;  /* ����Obj��ҳ�� */
    SPIFFS_SPAN_IX                  spanIXObjData = (pFd->uiSize > 0 ? pFd->uiSize - 1 : 0) / SPIFFS_DATA_PAGE_SIZE(pfs);
    UINT32                          uiCurSize = pFd->uiSize == (UINT32)SPIFFS_UNDEFINED_LEN ? 0 : pFd->uiSize;
    SPIFFS_SPAN_IX                  spanIXObjIXCur = 0;
    SPIFFS_SPAN_IX                  spanIXObjIXPrev = (SPIFFS_SPAN_IX)-1;
    PSPIFFS_PAGE_OBJECT_IX_HEADER   pObjIXHdr = (SPIFFS_PAGE_OBJECT_IX_HEADER *)pfs->pucWorkBuffer;
    SPIFFS_PAGE_OBJECT_IX           *pObjIX = (SPIFFS_PAGE_OBJECT_IX *)pfs->pucWorkBuffer;
    SPIFFS_PAGE_IX                  pageIXData;
    SPIFFS_PAGE_IX                  newPageIXObjIXHdr;

    // before truncating, check if object is to be fully removed and mark this
    /* ����Ƿ�ȫ��ɾ��������ǣ���������±��
       ~( SPIFFS_PH_FLAG_USED | SPIFFS_PH_FLAG_INDEX | SPIFFS_PH_FLAG_FINAL | SPIFFS_PH_FLAG_IXDELE ) 
    */
    if (bIsRemoveFull && uiNewSize == 0) {
        UINT8 flags = ~( SPIFFS_PH_FLAG_USED | SPIFFS_PH_FLAG_INDEX | SPIFFS_PH_FLAG_FINAL | SPIFFS_PH_FLAG_IXDELE);
        iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_UPDT, pFd->fileN, 
                                SPIFFS_PAGE_TO_PADDR(pfs, pFd->pageIXObjIXHdr) + offsetof(SPIFFS_PAGE_HEADER, flags),
                                sizeof(UINT8), (UINT8 *)&flags);
        SPIFFS_CHECK_RES(iRes);
    }

    // delete from end of object until desired uiLen is reached
    /* �Ӻ���ǰɾ */
    while (uiCurSize > uiNewSize) {
        spanIXObjIXCur = SPIFFS_OBJ_IX_ENTRY_SPAN_IX(pfs, spanIXObjData);      /* ��ǰ SpanIX Data ��Ӧ���Ǹ�Object IX��Span */
        // put object index for current data span index in pucWorkBuffer buffer
        if (spanIXObjIXPrev != spanIXObjIXCur) {            /* ֮ǰ�������ڵĲ�ͬ */
            if (spanIXObjIXPrev != (SPIFFS_SPAN_IX)-1) {    /* ǰһ��ҳ�����Чҳ�� */
                // remove previous object index page
                /* �Ƴ�ǰһҳ */
                SPIFFS_DBG("truncate: delete pObjIX page "_SPIPRIpg":"_SPIPRIsp"\n", pageIXObjIX, spanIXObjIXPrev);

                iRes = spiffsPageIndexCheck(pfs, pFd, pageIXObjIX, spanIXObjIXPrev);
                SPIFFS_CHECK_RES(iRes);

                iRes = spiffsPageDelete(pfs, pageIXObjIX);
                SPIFFS_CHECK_RES(iRes);
                spiffsCBObjectEvent(pfs, (SPIFFS_PAGE_OBJECT_IX *)0, SPIFFS_EV_IX_DEL, 
                                    pFd->objId, pObjIX->pageHdr.spanIX, pageIXObjIX, 0);
                if (spanIXObjIXPrev > 0) {  /* ��ͨIndex Pageͷ�� */
                    // Update object index header page, unless we totally want to remove the file.
                    // If fully removing, we're not keeping consistency as good as when storing the header between chunks,
                    // would we be aborted. But when removing full files, a crammed system may otherwise
                    // report ERR_FULL a la windows. We cannot have that.
                    // Hence, take the risk - if aborted, a file check would free the lost pages and mend things
                    // as the file is marked as fully deleted in the beginning.
                    if (bIsRemoveFull == LW_FALSE) {
                        SPIFFS_DBG("truncate: update pObjIX hdr page "_SPIPRIpg":"_SPIPRIsp" to uiSize "_SPIPRIi"\n", 
                                   pFd->pageIXObjIXHdr, spanIXObjIXPrev, uiCurSize);
                        iRes = spiffsObjectUpdateIndexHdr(pfs, pFd, pFd->objId, pFd->pageIXObjIXHdr, 
                                                          LW_NULL, LW_NULL, uiCurSize, &newPageIXObjIXHdr);
                        SPIFFS_CHECK_RES(iRes);
                    }
                    pFd->uiSize = uiCurSize;
                }
            }
            // load current object index (header) page
            /* ���ص�ǰIndex Page */
            if (spanIXObjIXCur == 0) {      /* ����ͷ��IndexPage */
                pageIXObjIX = pFd->pageIXObjIXHdr;
            }
            else {                          /* ��ͷ��IndexPage */
                /* ���ص�ǰҳ�� */
                iRes = spiffsObjLookUpFindIdAndSpan(pfs, pFd->objId | SPIFFS_OBJ_ID_IX_FLAG, 
                                                    spanIXObjIXCur, 0, &pageIXObjIX);
                SPIFFS_CHECK_RES(iRes);
            }

            SPIFFS_DBG("truncate: load pObjIX page "_SPIPRIpg":"_SPIPRIsp" for data spix:"_SPIPRIsp"\n", 
                       pageIXObjIX, spanIXObjIXCur, spanIXObjData);
            /* ����Work Buffer */
            iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_READ, pFd->fileN, 
                                   SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjIX), SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), 
                                   pfs->pucWorkBuffer);
            SPIFFS_CHECK_RES(iRes);
            SPIFFS_VALIDATE_OBJIX(pObjIXHdr->pageHdr, pFd->objId, spanIXObjIXCur);
            pFd->pageIXObjIXCursor = pageIXObjIX;
            pFd->spanIXObjIXCursor = spanIXObjIXCur;
            pFd->uiOffset = uiCurSize;

            spanIXObjIXPrev = spanIXObjIXCur;
        }

        //TODO: ΪʲôҪ���Ϊfree ? �ȱ��Free����һ���ٽ���ɾ����
        if (spanIXObjIXCur == 0) {      /* ��ǰҳ�����ײ�Index����SpanIndex = 0 */
            // get data page from object index header page
            pageIXData = ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIXHdr + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)))[spanIXObjData];
            ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIXHdr + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)))[spanIXObjData] = SPIFFS_OBJ_ID_FREE;
        } 
        else {
            // get data page from object index page
            pageIXData = ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIX + sizeof(SPIFFS_PAGE_OBJECT_IX)))[SPIFFS_OBJ_IX_ENTRY(pfs, spanIXObjData)];
            ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIX + sizeof(SPIFFS_PAGE_OBJECT_IX)))[SPIFFS_OBJ_IX_ENTRY(pfs, spanIXObjData)] = SPIFFS_OBJ_ID_FREE;
        }

        SPIFFS_DBG("truncate: got data pageIX "_SPIPRIpg"\n", pageIXData);

        if (uiNewSize == 0 || bIsRemoveFull 
         || uiCurSize - uiNewSize >= SPIFFS_DATA_PAGE_SIZE(pfs)) {
            // delete full data page
            //TODO��Ϊɶ���ﻹҪ��ɾ���أ�ǰ�治���Ѿ�ɾ����
            /* ����ɾ��һ����ҳ�� */
            iRes = spiffsPageDataCheck(pfs, pFd, pageIXData, spanIXObjData);
            if (iRes != SPIFFS_ERR_DELETED 
                && iRes != SPIFFS_OK 
                && iRes != SPIFFS_ERR_INDEX_REF_FREE) {
                SPIFFS_DBG("truncate: err validating data pageIX "_SPIPRIi"\n", iRes);
                break;
            }

            if (iRes == SPIFFS_OK) {
                iRes = spiffsPageDelete(pfs, pageIXData);
                if (iRes != SPIFFS_OK) {
                    SPIFFS_DBG("truncate: err deleting data pageIX "_SPIPRIi"\n", iRes);
                    break;
                }
            } 
            else if (iRes == SPIFFS_ERR_DELETED || iRes == SPIFFS_ERR_INDEX_REF_FREE) {
                iRes = SPIFFS_OK;
            }

            // update current uiSize
            /* ���µ�ǰ��С */
            if (uiCurSize % SPIFFS_DATA_PAGE_SIZE(pfs) == 0) {
                uiCurSize -= SPIFFS_DATA_PAGE_SIZE(pfs);
            } 
            else {
                uiCurSize -= uiCurSize % SPIFFS_DATA_PAGE_SIZE(pfs);
            }
            
            pFd->uiSize = uiCurSize;
            pFd->uiOffset = uiCurSize;
            SPIFFS_DBG("truncate: delete data page "_SPIPRIpg" for data spix:"_SPIPRIsp", uiCurSize:"_SPIPRIi"\n", 
                       pageIXData, spanIXObjData, uiCurSize);
        } 
        else {
            // delete last page, partially
            /* ���һ��ҳ�治һ��ȫ��ɾ�� */
            SPIFFS_PAGE_HEADER pageHeader;
            SPIFFS_PAGE_IX pageIXDataNew;
            UINT32 uiBytesToRemove = SPIFFS_DATA_PAGE_SIZE(pfs) - (uiNewSize % SPIFFS_DATA_PAGE_SIZE(pfs));
            
            SPIFFS_DBG("truncate: delete "_SPIPRIi" bytes from data page "_SPIPRIpg" for data spix:"_SPIPRIsp", uiCurSize:"_SPIPRIi"\n", 
                       uiBytesToRemove, pageIXData, spanIXObjData, uiCurSize);

            iRes = spiffsPageDataCheck(pfs, pFd, pageIXData, spanIXObjData);
            if (iRes != SPIFFS_OK) 
                break;

            pageHeader.objId = pFd->objId & ~SPIFFS_OBJ_ID_IX_FLAG;
            pageHeader.spanIX = spanIXObjData;
            pageHeader.flags = 0xff;
            // allocate new page and copy unmodified data
            //TODO: ֱ�Ӵ���pData��������
            iRes = spiffsPageAllocateData(pfs, pFd->objId & ~SPIFFS_OBJ_ID_IX_FLAG,
                                          &pageHeader, LW_NULL, 0, 0, LW_FALSE, &pageIXDataNew);
            if (iRes != SPIFFS_OK) 
                break;
            iRes = spiffsPhysCpy(pfs, 0,
                                 SPIFFS_PAGE_TO_PADDR(pfs, pageIXDataNew) + sizeof(SPIFFS_PAGE_HEADER),
                                 SPIFFS_PAGE_TO_PADDR(pfs, pageIXData) + sizeof(SPIFFS_PAGE_HEADER),
                                 SPIFFS_DATA_PAGE_SIZE(pfs) - uiBytesToRemove);
            if (iRes != SPIFFS_OK) 
                break;
            // delete original data page
            /* ֱ��ɾ��ԭ��������ҳ�棬��Ϊ���õ��Ѿ������� */
            iRes = spiffsPageDelete(pfs, pageIXData);
            if (iRes != SPIFFS_OK) 
                break;
            pageHeader.flags &= ~SPIFFS_PH_FLAG_FINAL;
            iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_UPDT, pFd->fileN,
                                    SPIFFS_PAGE_TO_PADDR(pfs, pageIXDataNew) + offsetof(SPIFFS_PAGE_HEADER, flags),
                                    sizeof(UINT8), (PCHAR)&pageHeader.flags);
            if (iRes != SPIFFS_OK) 
                break;

            // update memory representation of object index page with new data page
            /* �����ڴ� IX Obj */
            if (spanIXObjIXCur == 0) {
                /* �����IX OBJ�ĵ�0ҳ */
                // update object index header page
                ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIXHdr + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)))[spanIXObjData] = pageIXDataNew;
                SPIFFS_DBG("truncate: wrote page "_SPIPRIpg" to pObjIXHdr iEntry "_SPIPRIsp" in mem\n", 
                           pageIXDataNew, (SPIFFS_SPAN_IX)SPIFFS_OBJ_IX_ENTRY(pfs, spanIXObjData));
            } 
            else {
                /* �����IX OBJ�ķǵ�0ҳ */
                // update object index page
                ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIX + sizeof(SPIFFS_PAGE_OBJECT_IX)))[SPIFFS_OBJ_IX_ENTRY(pfs, spanIXObjData)] = pageIXDataNew;
                SPIFFS_DBG("truncate: wrote page "_SPIPRIpg" to pObjIX iEntry "_SPIPRIsp" in mem\n", pageIXDataNew, (SPIFFS_SPAN_IX)SPIFFS_OBJ_IX_ENTRY(pfs, spanIXObjData));
            }
            uiCurSize = uiNewSize;
            pFd->uiSize = uiNewSize;
            pFd->uiOffset = uiCurSize;
            break;
        }
        spanIXObjData--;
    } // while all data

    // update object indices
    if (spanIXObjIXCur == 0) {
        // update object index header page
        if (uiCurSize == 0) {
            if (bIsRemoveFull) {
                // remove object altogether
                SPIFFS_DBG("truncate: remove object index header page "_SPIPRIpg"\n", pageIXObjIX);

                iRes = spiffsPageIndexCheck(pfs, pFd, pageIXObjIX, 0);
                SPIFFS_CHECK_RES(iRes);

                iRes = spiffsPageDelete(pfs, pageIXObjIX);
                SPIFFS_CHECK_RES(iRes);
                spiffsCBObjectEvent(pfs, (SPIFFS_PAGE_OBJECT_IX *)0,
                                    SPIFFS_EV_IX_DEL, pFd->objId, 0, pageIXObjIX, 0);
            } 
            else {
                // make uninitialized object
                SPIFFS_DBG("truncate: reset pObjIXHdr page "_SPIPRIpg"\n", pageIXObjIX);
                memset(pfs->pucWorkBuffer + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER), 0xff,
                       SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) - sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER));
                iRes = spiffsObjectUpdateIndexHdr(pfs, pFd, pFd->objId, pageIXObjIX, 
                                                  pfs->pucWorkBuffer, LW_NULL, SPIFFS_UNDEFINED_LEN, 
                                                  &newPageIXObjIXHdr);
                SPIFFS_CHECK_RES(iRes);
            }
        } 
        else {
            // update object index header page
            SPIFFS_DBG("truncate: update object index header page with indices and uiSize\n");
            iRes = spiffsObjectUpdateIndexHdr(pfs, pFd, pFd->objId, pageIXObjIX, pfs->pucWorkBuffer, 
                                              LW_NULL, uiCurSize, &newPageIXObjIXHdr);
            SPIFFS_CHECK_RES(iRes);
        }
    } 
    else {
        // update both current object index page and object index header page
        SPIFFS_PAGE_IX pageIXObjIXNew;

        iRes = spiffsPageIndexCheck(pfs, pFd, pageIXObjIX, spanIXObjIXCur);
        SPIFFS_CHECK_RES(iRes);

        // move and update object index page
        iRes = spiffsPageMove(pfs, pFd->fileN, (UINT8*)pObjIXHdr, pFd->objId, LW_NULL, 
                              pageIXObjIX, &pageIXObjIXNew);
        SPIFFS_CHECK_RES(iRes);
        spiffsCBObjectEvent(pfs, (SPIFFS_PAGE_OBJECT_IX *)pObjIXHdr, SPIFFS_EV_IX_UPD, 
                            pFd->objId, pObjIX->pageHdr.spanIX, pageIXObjIXNew, 0);
        SPIFFS_DBG("truncate: store modified pObjIX page, "_SPIPRIpg":"_SPIPRIsp"\n", pageIXObjIXNew, spanIXObjIXCur);
        pFd->pageIXObjIXCursor = pageIXObjIXNew;
        pFd->spanIXObjIXCursor = spanIXObjIXCur;
        pFd->uiOffset = uiCurSize;
        // update object index header page with new uiSize
        iRes = spiffsObjectUpdateIndexHdr(pfs, pFd, pFd->objId, pFd->pageIXObjIXHdr, 
                                          LW_NULL, LW_NULL, uiCurSize, &newPageIXObjIXHdr);
        SPIFFS_CHECK_RES(iRes);
    }
    pFd->uiSize = uiCurSize;
    return iRes;
}
//TODO:�����Ż� + ˼·����
/*********************************************************************************************************
** ��������: spiffsObjectAppend
** ��������: ����׷��дһ��Object
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjectAppend(PSPIFFS_FD pFd, UINT32 uiOffset, PCHAR pContent, UINT32 uiLen){
    PSPIFFS_VOLUME pfs = pFd->pfs;
    INT32 iRes = SPIFFS_OK;
    UINT32 uiByteHasWritten = 0;

    SPIFFS_DBG("append: "_SPIPRIi" bytes @ offs "_SPIPRIi" of uiSize "_SPIPRIi"\n", 
                uiLen, uiOffset, pFd->uiSize);

    if (uiOffset > pFd->uiSize) {
        SPIFFS_DBG("append: uiOffset reversed to uiSize\n");
        uiOffset = pFd->uiSize;
    }

    iRes = spiffsGCCheck(pfs, uiLen + SPIFFS_DATA_PAGE_SIZE(pfs)); // add an extra page of data worth for meta
    if (iRes != SPIFFS_OK) {
        SPIFFS_DBG("append: gc check fail "_SPIPRIi"\n", iRes);
    }
    SPIFFS_CHECK_RES(iRes);

    PSPIFFS_PAGE_OBJECT_IX_HEADER objIXHdr = (PSPIFFS_PAGE_OBJECT_IX_HEADER)pfs->pucWorkBuffer;
    PSPIFFS_PAGE_OBJECT_IX objIX = (PSPIFFS_PAGE_OBJECT_IX)pfs->pucWorkBuffer;
    SPIFFS_PAGE_HEADER pageHeader;

    SPIFFS_SPAN_IX spanIXObjIXCur = 0;
    SPIFFS_SPAN_IX spanIXObjIXPrev = (SPIFFS_SPAN_IX)-1;
    SPIFFS_PAGE_IX pageIXObjIXCur = pFd->pageIXObjIXHdr;
    SPIFFS_PAGE_IX pageIXObjIXHdrNew;

    SPIFFS_SPAN_IX spanIXObjData = uiOffset / SPIFFS_DATA_PAGE_SIZE(pfs);
    SPIFFS_PAGE_IX pageIXObjData;
    UINT32 uiPageOffset = uiOffset % SPIFFS_DATA_PAGE_SIZE(pfs);

    // write all data
    while (iRes == SPIFFS_OK && uiByteHasWritten < uiLen) {
        // calculate object index page span index
        spanIXObjIXCur = SPIFFS_OBJ_IX_ENTRY_SPAN_IX(pfs, spanIXObjData);   /* ���ݵ�ǰDataҳ���SpanIX��ȡ��ǰIndexҳ���SpanIX */

        // handle storing and loading of object indices
        if (spanIXObjIXCur != spanIXObjIXPrev) {
            // new object index page
            // within this clause we return directly if something fails, object index mess-up
            if (uiByteHasWritten > 0) {
                // store previous object index page, unless first pass
                SPIFFS_DBG("append: "_SPIPRIid" store objIX "_SPIPRIpg":"_SPIPRIsp", uiByteHasWritten "_SPIPRIi"\n", 
                            pFd->objId,pageIXObjIXCur, spanIXObjIXPrev, uiByteHasWritten);
                if (spanIXObjIXPrev == 0) {         /* ��һ��Indexҳ���SpanIX = 0*/
                    // this is an update to object index header page
                    objIXHdr->uiSize = uiOffset + uiByteHasWritten;
                    if (uiOffset == 0) {
                        // was an empty object, update same page (uiSize was 0xffffffff)
                        iRes = spiffsPageIndexCheck(pfs, pFd, pageIXObjIXCur, 0);
                        SPIFFS_CHECK_RES(iRes);
                        iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_UPDT, pFd->fileN, 
                                                SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjIXCur), 
                                                SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), pfs->pucWorkBuffer);
                        SPIFFS_CHECK_RES(iRes);
                    } 
                    else {
                        // was a nonempty object, update to new page
                        iRes = spiffsObjectUpdateIndexHdr(pfs, pFd, pFd->objId, pFd->pageIXObjIXHdr, 
                                                          pfs->pucWorkBuffer, LW_NULL, uiOffset + uiByteHasWritten, 
                                                          &pageIXObjIXHdrNew);
                        SPIFFS_CHECK_RES(iRes);
                        SPIFFS_DBG("append: "_SPIPRIid" store new objIXHdr, "_SPIPRIpg":"_SPIPRIsp", uiByteHasWritten "_SPIPRIi"\n", 
                                    pFd->objId, pageIXObjIXHdrNew, 0, uiByteHasWritten);
                    }
                } 
                else {                              /* ��һ��Indexҳ���SpanIX != 0*/
                    // this is an update to an object index page
                    iRes = spiffsPageIndexCheck(pfs, pFd, pageIXObjIXCur, spanIXObjIXPrev);
                    SPIFFS_CHECK_RES(iRes);

                    iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_UPDT, pFd->fileN, 
                                            SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjIXCur), 
                                            SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), pfs->pucWorkBuffer);
                    SPIFFS_CHECK_RES(iRes);
                    spiffsCBObjectEvent(pfs, (PSPIFFS_PAGE_OBJECT_IX)pfs->pucWorkBuffer, SPIFFS_EV_IX_UPD,
                                        pFd->objId, objIX->pageHdr.spanIX, pageIXObjIXCur, 0);
                    // update length in object index header page
                    iRes = spiffsObjectUpdateIndexHdr(pfs, pFd, pFd->objId, pFd->pageIXObjIXHdr, 
                                                      LW_NULL, LW_NULL, 
                                                      uiOffset + uiByteHasWritten, &pageIXObjIXHdrNew);
                    SPIFFS_CHECK_RES(iRes);
                    SPIFFS_DBG("append: "_SPIPRIid" store new uiSize I "_SPIPRIi" in objIXHdr, "_SPIPRIpg":"_SPIPRIsp", uiByteHasWritten "_SPIPRIi"\n", 
                                pFd->objId, uiOffset + uiByteHasWritten, pageIXObjIXHdrNew, 0, uiByteHasWritten);
                }
                pFd->uiSize = uiOffset + uiByteHasWritten;
                pFd->uiOffset = uiOffset + uiByteHasWritten;
            }

            // create or load new object index page
            if (spanIXObjIXCur == 0) {      /* ��ǰIndex ҳ���SpanIX = 0 */
                // load object index header page, must always exist
                SPIFFS_DBG("append: "_SPIPRIid" load objixhdr page "_SPIPRIpg":"_SPIPRIsp"\n", 
                            pFd->objId, pageIXObjIXCur, spanIXObjIXCur);
                /* ������ؽ��ڴ� */    
                iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_READ, pFd->fileN, 
                                    SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjIXCur), 
                                    SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), pfs->pucWorkBuffer);
                SPIFFS_CHECK_RES(iRes);
                SPIFFS_VALIDATE_OBJIX(objIXHdr->pageHdr, pFd->objId, spanIXObjIXCur);
            }   
            else {                          /* ��ǰIndex ҳ���SpanIX != 0 */
                SPIFFS_SPAN_IX spanIXObjIXLen = SPIFFS_OBJ_IX_ENTRY_SPAN_IX(pfs, (pFd->uiSize - 1) / SPIFFS_DATA_PAGE_SIZE(pfs));
                // on subsequent passes, create a new object index page
                if (uiByteHasWritten > 0 || spanIXObjIXCur > spanIXObjIXLen) {
                    pageHeader.objId = pFd->objId | SPIFFS_OBJ_ID_IX_FLAG;
                    pageHeader.spanIX = spanIXObjIXCur;
                    pageHeader.flags = 0xff & ~(SPIFFS_PH_FLAG_FINAL | SPIFFS_PH_FLAG_INDEX);
                    iRes = spiffsPageAllocateData(pfs, pFd->objId | SPIFFS_OBJ_ID_IX_FLAG, &pageHeader, 
                                                  LW_NULL, 0, 0, LW_TRUE, &pageIXObjIXCur);
                    SPIFFS_CHECK_RES(iRes);
                    // quick "load" of new object index page
                    memset(pfs->pucWorkBuffer, 0xff, SPIFFS_CFG_LOGIC_PAGE_SZ(pfs));
                    lib_memcpy(pfs->pucWorkBuffer, &pageHeader, sizeof(SPIFFS_PAGE_HEADER));
                    spiffsCBObjectEvent(pfs, (PSPIFFS_PAGE_OBJECT_IX)pfs->pucWorkBuffer, SPIFFS_EV_IX_NEW, 
                                        pFd->objId, spanIXObjIXCur, pageIXObjIXCur, 0);
                    SPIFFS_DBG("append: "_SPIPRIid" create objIX page, "_SPIPRIpg":"_SPIPRIsp", uiByteHasWritten "_SPIPRIi"\n", 
                                pFd->objId, pageIXObjIXCur, spanIXObjIXCur, uiByteHasWritten);
                } 
                else {
                    // on first pass, we load existing object index page
                    SPIFFS_PAGE_IX pageIX;
                    SPIFFS_DBG("append: "_SPIPRIid" find objIX spanIX:"_SPIPRIsp"\n", 
                                pFd->objId, spanIXObjIXCur);
                    if (pFd->spanIXObjIXCursor == spanIXObjIXCur) {
                        pageIX = pFd->pageIXObjIXCursor;
                    } 
                    else {
                        iRes = spiffsObjLookUpFindIdAndSpan(pfs, pFd->objId | SPIFFS_OBJ_ID_IX_FLAG, 
                                                            spanIXObjIXCur, 0, &pageIX);
                        SPIFFS_CHECK_RES(iRes);
                    }
                    SPIFFS_DBG("append: "_SPIPRIid" found object index at page "_SPIPRIpg" [pFd uiSize "_SPIPRIi"]\n", 
                                pFd->objId, pageIX, pFd->uiSize);
                    iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_READ, pFd->fileN, 
                                           SPIFFS_PAGE_TO_PADDR(pfs, pageIX), 
                                           SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), pfs->pucWorkBuffer);
                    SPIFFS_CHECK_RES(iRes);
                    SPIFFS_VALIDATE_OBJIX(objIXHdr->pageHdr, pFd->objId, spanIXObjIXCur);
                    pageIXObjIXCur = pageIX;
                }
                pFd->pageIXObjIXCursor = pageIXObjIXCur;
                pFd->spanIXObjIXCursor = spanIXObjIXCur;
                pFd->uiOffset          = uiOffset+uiByteHasWritten;
                pFd->uiSize            = uiOffset+uiByteHasWritten;
            }
            spanIXObjIXPrev = spanIXObjIXCur;
        }

        // write data
        UINT32 uiByteToWrite = MIN(uiLen - uiByteHasWritten, SPIFFS_DATA_PAGE_SIZE(pfs) - uiPageOffset);
        if (uiPageOffset == 0) {
            /* ֱ��д��һ���µ�����ҳ */
            // at beginning of a page, allocate and write a new page of data
            pageHeader.objId = pFd->objId & ~SPIFFS_OBJ_ID_IX_FLAG;
            pageHeader.spanIX = spanIXObjData;
            pageHeader.flags = 0xff & ~(SPIFFS_PH_FLAG_FINAL);  // finalize immediately
            iRes = spiffsPageAllocateData(pfs, pFd->objId & ~SPIFFS_OBJ_ID_IX_FLAG, &pageHeader, 
                                          &pContent[uiByteHasWritten], uiByteToWrite, uiPageOffset, 
                                          LW_TRUE, &pageIXObjData);
            SPIFFS_DBG("append: "_SPIPRIid" store new data page, "_SPIPRIpg":"_SPIPRIsp" uiOffset:"_SPIPRIi", uiLen "_SPIPRIi", uiByteHasWritten "_SPIPRIi"\n",
                        pFd->objId,pageIXObjData, spanIXObjData, uiPageOffset, uiByteToWrite, uiByteHasWritten);
        } 
        else {
            // append to existing page, fill out free data in existing page
            /* ׷��д�����е�����ҳ֮�� */
            if (spanIXObjIXCur == 0) {
                // get data page from object index header page
                pageIXObjData = ((SPIFFS_PAGE_IX*)((PCHAR)objIXHdr + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)))[spanIXObjData];
            } 
            else {
                // get data page from object index page
                pageIXObjData = ((SPIFFS_PAGE_IX*)((PCHAR)objIX + sizeof(SPIFFS_PAGE_OBJECT_IX)))[SPIFFS_OBJ_IX_ENTRY(pfs, spanIXObjData)];
            }

            iRes = spiffsPageDataCheck(pfs, pFd, pageIXObjData, spanIXObjData);
            SPIFFS_CHECK_RES(iRes);

            iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_UPDT, pFd->fileN, 
                                    SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjData) + sizeof(SPIFFS_PAGE_HEADER) + uiPageOffset, 
                                    uiByteToWrite, &pContent[uiByteHasWritten]);
            SPIFFS_DBG("append: "_SPIPRIid" store to existing data page, "_SPIPRIpg":"_SPIPRIsp" uiOffset:"_SPIPRIi", uiLen "_SPIPRIi", uiByteHasWritten "_SPIPRIi"\n", 
                        pFd->objId , pageIXObjData, spanIXObjData, uiPageOffset, uiByteToWrite, uiByteHasWritten);
        }

        if (iRes != SPIFFS_OK) 
            break;

        // update memory representation of object index page with new data page
        /* ���ݵ�ǰ�ڴ��л����Indexҳ��������Entry */
        if (spanIXObjIXCur == 0) {
            // update object index header page
            ((SPIFFS_PAGE_IX*)((PCHAR)objIXHdr + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)))[spanIXObjData] = pageIXObjData;
            SPIFFS_DBG("append: "_SPIPRIid" wrote page "_SPIPRIpg" to objIXHdr entry "_SPIPRIsp" in mem\n", 
                        pFd->objId, pageIXObjData, spanIXObjData);
            objIXHdr->uiSize = uiOffset + uiByteHasWritten;
        } 
        else {
            // update object index page
            ((SPIFFS_PAGE_IX*)((PCHAR)objIX + sizeof(SPIFFS_PAGE_OBJECT_IX)))[SPIFFS_OBJ_IX_ENTRY(pfs, spanIXObjData)] = pageIXObjData;
            SPIFFS_DBG("append: "_SPIPRIid" wrote page "_SPIPRIpg" to objIX entry "_SPIPRIsp" in mem\n", 
                        pFd->objId, pageIXObjData, (SPIFFS_SPAN_IX)SPIFFS_OBJ_IX_ENTRY(pfs, spanIXObjData));
        }

        // update internals
        uiPageOffset = 0;
        spanIXObjData++;
        uiByteHasWritten += uiByteToWrite;
    } // while all data

    pFd->uiSize = uiOffset + uiByteHasWritten;
    pFd->uiOffset = uiOffset + uiByteHasWritten;
    pFd->pageIXObjIXCursor = pageIXObjIXCur;    /* ���µ�ǰ��дָ�� */
    pFd->spanIXObjIXCursor = spanIXObjIXCur;

    // finalize updated object indices
    INT32 iRes2 = SPIFFS_OK;
    /* ����ٴθ������� */
    if (spanIXObjIXCur != 0) {
        // wrote beyond object index header page
        // write last modified object index page, unless object header index page
        SPIFFS_DBG("append: "_SPIPRIid" store objIX page, "_SPIPRIpg":"_SPIPRIsp", uiByteHasWritten "_SPIPRIi"\n", 
                    pFd->objId,pageIXObjIXCur, spanIXObjIXCur, uiByteHasWritten);

        iRes2 = spiffsPageIndexCheck(pfs, pFd, pageIXObjIXCur, spanIXObjIXCur);
        SPIFFS_CHECK_RES(iRes2);

        iRes2 = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_UPDT, pFd->fileN, 
                                 SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjIXCur), 
                                 SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), pfs->pucWorkBuffer);
        SPIFFS_CHECK_RES(iRes2);
        spiffsCBObjectEvent(pfs, (PSPIFFS_PAGE_OBJECT_IX)pfs->pucWorkBuffer, SPIFFS_EV_IX_UPD, 
                            pFd->objId, objIX->pageHdr.spanIX, pageIXObjIXCur, 0);

        // update uiSize in object header index page
        iRes2 = spiffsObjectUpdateIndexHdr(pfs, pFd, pFd->objId, pFd->pageIXObjIXHdr, 
                                          LW_NULL, LW_NULL, uiOffset+uiByteHasWritten, &pageIXObjIXHdrNew);
        SPIFFS_DBG("append: "_SPIPRIid" store new uiSize II "_SPIPRIi" in objIXHdr, "_SPIPRIpg":"_SPIPRIsp", uiByteHasWritten "_SPIPRIi", iRes "_SPIPRIi"\n", 
                    pFd->objId, uiOffset + uiByteHasWritten, pageIXObjIXHdrNew, 0, uiByteHasWritten, iRes2);
        SPIFFS_CHECK_RES(iRes2);
    } 
    else {  /* spanIXObjIXCur == 0 */
        // wrote within object index header page
        if (uiOffset == 0) {
            // wrote to empty object - simply update uiSize and write whole page
            /* uiOffset = 0Ԥʾ����һ���µ�Obj��������ǵ���CacheWrite���� */
            objIXHdr->uiSize = uiOffset + uiByteHasWritten;
            SPIFFS_DBG("append: "_SPIPRIid" store fresh objIXHdr page, "_SPIPRIpg":"_SPIPRIsp", uiByteHasWritten "_SPIPRIi"\n", 
                        pFd->objId, pageIXObjIXCur, spanIXObjIXCur, uiByteHasWritten);

            iRes2 = spiffsPageIndexCheck(pfs, pFd, pageIXObjIXCur, spanIXObjIXCur);
            SPIFFS_CHECK_RES(iRes2);

            iRes2 = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_UPDT, pFd->fileN, 
                                     SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjIXCur), 
                                     SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), pfs->pucWorkBuffer);
            SPIFFS_CHECK_RES(iRes2);
            // callback on object index update
            spiffsCBObjectEvent(pfs, (PSPIFFS_PAGE_OBJECT_IX)pfs->pucWorkBuffer, SPIFFS_EV_IX_UPD_HDR, 
                                pFd->objId, objIXHdr->pageHdr.spanIX, pageIXObjIXCur, objIXHdr->uiSize);
        } 
        else {
            /* uiOffset != 0Ԥʾ����һ���Ѿ����ڵ�Obj��������Ҫ��ظ������Ĵ�С����˵���spiffsObjectUpdateIndexHdr */
            // modifying object index header page, update uiSize and make new copy
            iRes2 = spiffsObjectUpdateIndexHdr(pfs, pFd, pFd->objId, pFd->pageIXObjIXHdr, 
                                               pfs->pucWorkBuffer, LW_NULL, 
                                               uiOffset + uiByteHasWritten, &pageIXObjIXHdrNew);
            SPIFFS_DBG("append: "_SPIPRIid" store modified objIXHdr page, "_SPIPRIpg":"_SPIPRIsp", uiByteHasWritten "_SPIPRIi"\n", 
                        pFd->objId , pageIXObjIXHdrNew, 0, uiByteHasWritten);
            SPIFFS_CHECK_RES(iRes2);
        }
    }

    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsObjectModify
** ��������: ���ڽض�һ��Object
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjectModify(PSPIFFS_FD pFd, UINT32 uiOffset, PCHAR pContent, UINT32 uiLen){
    PSPIFFS_VOLUME pfs = pFd->pfs;
    INT32 iRes = SPIFFS_OK;
    UINT32 uiByteHasWritten = 0;

    iRes = spiffsGCCheck(pfs, uiLen + SPIFFS_DATA_PAGE_SIZE(pfs));
    SPIFFS_CHECK_RES(iRes);
    
    PSPIFFS_PAGE_OBJECT_IX_HEADER objIXHdr = (PSPIFFS_PAGE_OBJECT_IX_HEADER)pfs->pucWorkBuffer; /* SpanIXΪ0������ҳ��Header */
    PSPIFFS_PAGE_OBJECT_IX        objIX = (PSPIFFS_PAGE_OBJECT_IX)pfs->pucWorkBuffer;           /* SpanIX��0������ҳ��Header */
    SPIFFS_PAGE_HEADER            pageHeader;                                                   /* ��ͨҳ���Header */

    SPIFFS_SPAN_IX spanIXObjIXCur = 0;                                                          /* ��ǰIndexҳ���Span IX */
    SPIFFS_SPAN_IX spanIXObjIXPrev = (SPIFFS_SPAN_IX)-1;                                        /* ǰһ��Indexҳ���Span IX */
    SPIFFS_PAGE_IX pageIXObjIXCur = pFd->pageIXObjIXHdr;                                        /* ��ǰIndexҳ���Page IX */
    SPIFFS_PAGE_IX pageIXObjIXHdrNew;                                                           /* ���µ� SpanIX = 0��Indexҳ���SpanIX */
    SPIFFS_PAGE_IX pageIXObjIXNew;                                                              /* ���µ� SpanIX != 0��Indexҳ���SpanIX */
    
    SPIFFS_SPAN_IX spanIXObjData = uiOffset / SPIFFS_DATA_PAGE_SIZE(pfs);                       /* Dataҳ���SpanIX */
    SPIFFS_PAGE_IX pageIXObjData;                                                               /* Dataҳ���PageIX */                                
    UINT32 uiPageOffset = uiOffset % SPIFFS_DATA_PAGE_SIZE(pfs);                                /* Dataҳ��ҳ��ƫ�� */


    // write all data
    while (iRes == SPIFFS_OK 
           && uiByteHasWritten < uiLen) {
        // calculate object index page span index
        spanIXObjIXCur = SPIFFS_OBJ_IX_ENTRY_SPAN_IX(pfs, spanIXObjData);                       /* ����Dataҳ���spanIX���㵱ǰIndexҳ���spanIX */

        // handle storing and loading of object indices
        if (spanIXObjIXCur != spanIXObjIXPrev) {                                                /* ��ǰIndexҳ���spanIX����һ��Inxdexҳ��spanIX��һ�� */
            // new object index page
            // within this clause we return directly if something fails, object index mess-up
            if (uiByteHasWritten > 0) {
                // store previous object index (header) page, unless first pass
                if (spanIXObjIXPrev == 0) {                                                     /* ��һ��Indexҳ���SpanIX = 0 */
                    // store previous object index header page
                    iRes = spiffsObjectUpdateIndexHdr(pfs, pFd, pFd->objId, pFd->pageIXObjIXHdr, 
                                                      pfs->pucWorkBuffer, LW_NULL, 0, 
                                                      &pageIXObjIXHdrNew);
                    SPIFFS_DBG("modify: store modified objIXHdr page, "_SPIPRIpg":"_SPIPRIsp", uiByteHasWritten "_SPIPRIi"\n", 
                                pageIXObjIXHdrNew, 0, uiByteHasWritten);
                    SPIFFS_CHECK_RES(iRes);
                } 
                else {
                    // store new version of previous object index page
                    iRes = spiffsPageIndexCheck(pfs, pFd, pageIXObjIXCur, spanIXObjIXPrev);
                    SPIFFS_CHECK_RES(iRes);
                    
                    iRes = spiffsPageMove(pfs, pFd->fileN, (PCHAR)objIX, pFd->objId, 
                                          0, pageIXObjIXCur, &pageIXObjIXNew);
                    SPIFFS_DBG("modify: store previous modified objIX page, "_SPIPRIid":"_SPIPRIsp", uiByteHasWritten "_SPIPRIi"\n", 
                                pageIXObjIXNew, objIX->pageHdr.spanIX, uiByteHasWritten);
                    SPIFFS_CHECK_RES(iRes);
                    spiffsCBObjectEvent(pfs, (PSPIFFS_PAGE_OBJECT_IX)objIX,
                                        SPIFFS_EV_IX_UPD, pFd->objId, 
                                        objIX->pageHdr.spanIX, pageIXObjIXNew, 0);
                }
            }
            // load next object index page
            if (spanIXObjIXCur == 0) {
                // load object index header page, must exist
                SPIFFS_DBG("modify: load objixhdr page "_SPIPRIpg":"_SPIPRIsp"\n", 
                            pageIXObjIXCur, spanIXObjIXCur);
                iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_READ, pFd->fileN, 
                                        SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjIXCur), 
                                        SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), pfs->pucWorkBuffer);
                SPIFFS_CHECK_RES(iRes);
                SPIFFS_VALIDATE_OBJIX(objIXHdr->pageHdr, pFd->objId, spanIXObjIXCur);
            } 
            else {
                // load existing object index page on first pass
                SPIFFS_PAGE_IX pageIX;
                SPIFFS_DBG("modify: find objIX spanIX:"_SPIPRIsp"\n", spanIXObjIXCur);
                if (pFd->spanIXObjIXCursor == spanIXObjIXCur) {
                    pageIX = pFd->pageIXObjIXCursor;
                } 
                else {
                    iRes = spiffsObjLookUpFindIdAndSpan(pfs, pFd->objId | SPIFFS_OBJ_ID_IX_FLAG, 
                                                        spanIXObjIXCur, 0, &pageIX);
                    SPIFFS_CHECK_RES(iRes);
                }
                SPIFFS_DBG("modify: found object index at page "_SPIPRIpg"\n", pageIX);
                iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_READ, pFd->fileN, 
                                       SPIFFS_PAGE_TO_PADDR(pfs, pageIX), 
                                       SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), pfs->pucWorkBuffer);
                SPIFFS_CHECK_RES(iRes);
                SPIFFS_VALIDATE_OBJIX(objIXHdr->pageHdr, pFd->objId, spanIXObjIXCur);
                pageIXObjIXCur = pageIX;
            }
            
            pFd->pageIXObjIXCursor = pageIXObjIXCur;
            pFd->spanIXObjIXCursor = spanIXObjIXCur;
            pFd->uiOffset = uiOffset + uiByteHasWritten;
            spanIXObjIXPrev = spanIXObjIXCur;
        }

        // write partial data
        UINT32         uiByteToWrite = MIN(uiLen - uiByteHasWritten, SPIFFS_DATA_PAGE_SIZE(pfs) - uiPageOffset);
        SPIFFS_PAGE_IX pageIXObjDataOrigin;
        if (spanIXObjIXCur == 0) {
            // get data page from object index header page
            pageIXObjDataOrigin = ((SPIFFS_PAGE_IX *)((PCHAR)objIXHdr + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)))[spanIXObjData];
        } 
        else {
            // get data page from object index page
            pageIXObjDataOrigin = ((SPIFFS_PAGE_IX*)((PCHAR)objIX + sizeof(SPIFFS_PAGE_OBJECT_IX)))[SPIFFS_OBJ_IX_ENTRY(pfs, spanIXObjData)];
        }

        pageHeader.objId = pFd->objId & ~SPIFFS_OBJ_ID_IX_FLAG;
        pageHeader.spanIX = spanIXObjData;
        pageHeader.flags = 0xff;
        if (uiPageOffset == 0 && 
            uiByteToWrite == SPIFFS_DATA_PAGE_SIZE(pfs)) {
            // a full page, allocate and write a new page of data
            /* ����д��һ��ҳ�� */
            iRes = spiffsPageAllocateData(pfs, pageHeader.objId, &pageHeader, 
                                          &pContent[uiByteHasWritten], uiByteToWrite, 
                                          uiPageOffset, LW_TRUE, &pageIXObjData);
            SPIFFS_DBG("modify: store new data page, "_SPIPRIpg":"_SPIPRIsp" uiOffset:"_SPIPRIi", uiLen "_SPIPRIi", uiByteHasWritten "_SPIPRIi"\n", 
                        pageIXObjData, spanIXObjData, uiPageOffset, uiByteToWrite, uiByteHasWritten);
        } 
        else {
            // write to existing page, allocate new and copy unmodified data

            iRes = spiffsPageDataCheck(pfs, pFd, pageIXObjDataOrigin, spanIXObjData);
            SPIFFS_CHECK_RES(iRes);
            /* ����һ������ҳ�� */
            iRes = spiffsPageAllocateData(pfs, pageHeader.objId, &pageHeader, 
                                          LW_NULL, 0, 0, LW_FALSE, &pageIXObjData);
            if (iRes != SPIFFS_OK) 
                break;

            // copy unmodified data
            if (uiPageOffset > 0) {
                // before modification
                iRes = spiffsPhysCpy(pfs, pFd->fileN,
                                     SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjData) + sizeof(SPIFFS_PAGE_HEADER),
                                     SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjDataOrigin) + sizeof(SPIFFS_PAGE_HEADER),
                                     uiPageOffset);
                if (iRes != SPIFFS_OK) 
                    break;
            }
            if (uiPageOffset + uiByteToWrite < SPIFFS_DATA_PAGE_SIZE(pfs)) {
                // after modification
                iRes = spiffsPhysCpy(pfs, pFd->fileN,
                                     SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjData) + sizeof(SPIFFS_PAGE_HEADER) + uiPageOffset + uiByteToWrite,
                                     SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjDataOrigin) + sizeof(SPIFFS_PAGE_HEADER) + uiPageOffset + uiByteToWrite,
                                     SPIFFS_DATA_PAGE_SIZE(pfs) - (uiPageOffset + uiByteToWrite));
                if (iRes != SPIFFS_OK) 
                    break;
            }

            iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_UPDT, pFd->fileN,
                                    SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjData) + sizeof(SPIFFS_PAGE_HEADER) + uiPageOffset, 
                                    uiByteToWrite, &pContent[uiByteHasWritten]);
            if (iRes != SPIFFS_OK) 
                break;

            pageHeader.flags &= ~SPIFFS_PH_FLAG_FINAL;
            iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_UPDT, pFd->fileN,
                                    SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjData) + offsetof(SPIFFS_PAGE_HEADER, flags),
                                    sizeof(UINT8), (PCHAR)&pageHeader.flags);
            if (iRes != SPIFFS_OK) 
                break;

            SPIFFS_DBG("modify: store to existing data page, src:"_SPIPRIpg", dst:"_SPIPRIpg":"_SPIPRIsp" uiOffset:"_SPIPRIi", uiLen "_SPIPRIi", uiByteHasWritten "_SPIPRIi"\n", 
                        pageIXObjDataOrigin, pageIXObjData, spanIXObjData, uiPageOffset, 
                        uiByteToWrite, uiByteHasWritten);
        }

        // delete original data page
        iRes = spiffsPageDelete(pfs, pageIXObjDataOrigin);
        if (iRes != SPIFFS_OK) 
            break;
        // update memory representation of object index page with new data page
        if (spanIXObjIXCur == 0) {
            // update object index header page
            ((SPIFFS_PAGE_IX*)((PCHAR)objIXHdr + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)))[spanIXObjData] = pageIXObjData;
            SPIFFS_DBG("modify: wrote page "_SPIPRIpg" to objIXHdr entry "_SPIPRIsp" in mem\n", pageIXObjData, spanIXObjData);
        } else {
            // update object index page
            ((SPIFFS_PAGE_IX*)((PCHAR)objIX + sizeof(SPIFFS_PAGE_OBJECT_IX)))[SPIFFS_OBJ_IX_ENTRY(pfs, spanIXObjData)] = pageIXObjData;
            SPIFFS_DBG("modify: wrote page "_SPIPRIpg" to objIX entry "_SPIPRIsp" in mem\n", pageIXObjData, (SPIFFS_SPAN_IX)SPIFFS_OBJ_IX_ENTRY(pfs, spanIXObjData));
        }

        // update internals
        uiPageOffset = 0;
        spanIXObjData++;
        uiByteHasWritten += uiByteToWrite;
    } // while all data

    pFd->uiOffset = uiOffset + uiByteHasWritten;
    pFd->pageIXObjIXCursor = pageIXObjIXCur;
    pFd->spanIXObjIXCursor = spanIXObjIXCur;

    // finalize updated object indices
    INT32 iRes2 = SPIFFS_OK;
    if (spanIXObjIXCur != 0) {
        // wrote beyond object index header page
        // write last modified object index page
        // move and update page
        SPIFFS_PAGE_IX pageIXObjIXNew;

        iRes2 = spiffsPageIndexCheck(pfs, pFd, pageIXObjIXCur, spanIXObjIXCur);
        SPIFFS_CHECK_RES(iRes2);

        iRes2 = spiffsPageMove(pfs, pFd->fileN, (PCHAR)objIX, pFd->objId, LW_NULL, 
                               pageIXObjIXCur, &pageIXObjIXNew);
        SPIFFS_DBG("modify: store modified objIX page, "_SPIPRIpg":"_SPIPRIsp", uiByteHasWritten "_SPIPRIi"\n", pageIXObjIXNew, spanIXObjIXCur, uiByteHasWritten);
        pFd->pageIXObjIXCursor = pageIXObjIXNew;
        pFd->spanIXObjIXCursor = spanIXObjIXCur;
        SPIFFS_CHECK_RES(iRes2);
        spiffsCBObjectEvent(pfs, (PSPIFFS_PAGE_OBJECT_IX)objIX, SPIFFS_EV_IX_UPD, 
                            pFd->objId, objIX->pageHdr.spanIX, pageIXObjIXNew, 0);

    } 
    else {
        // wrote within object index header page
        iRes2 = spiffsObjectUpdateIndexHdr(pfs, pFd, pFd->objId, pFd->pageIXObjIXHdr, 
                                           pfs->pucWorkBuffer, LW_NULL, 0, &pageIXObjIXHdrNew);
        SPIFFS_DBG("modify: store modified objIXHdr page, "_SPIPRIpg":"_SPIPRIsp", uiByteHasWritten "_SPIPRIi"\n", 
                    pageIXObjIXHdrNew, 0, uiByteHasWritten);
        SPIFFS_CHECK_RES(iRes2);
    }

    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsObjectRead
** ��������: ��һ��Object�ж�ȡ������puDst
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjectRead(PSPIFFS_FD pFd, UINT32 uiOffset, UINT32 uiLen, PCHAR puDst){
    INT32          iRes = SPIFFS_OK;
    PSPIFFS_VOLUME pfs = pFd->pfs;                                               /* �ļ�ͷ */                                                   
    SPIFFS_PAGE_IX pageIXObjIX;                                                 /* ��ǰIndexҳ���page index */
    SPIFFS_PAGE_IX pageIXObjData;                                               /* ��ǰDataҳ���page index */
    SPIFFS_SPAN_IX spanIXObjData = uiOffset / SPIFFS_DATA_PAGE_SIZE(pfs);       /* ��ǰDataҳ���Span index */
    UINT32         uiCurOffset = uiOffset;                                      /* ��ǰ�ļ�Obj�Ķ�ƫ�� */
    SPIFFS_SPAN_IX spanIXObjIXCur;                                              /* ��ǰIndexҳ���Span Index */
    SPIFFS_SPAN_IX spanIXObjIXPrev = (SPIFFS_SPAN_IX)-1;                        /* ��ǰIndexҳ���Span Index */
    
    PSPIFFS_PAGE_OBJECT_IX_HEADER objIXHdr = (PSPIFFS_PAGE_OBJECT_IX_HEADER)pfs->pucWorkBuffer;
    PSPIFFS_PAGE_OBJECT_IX        objIX = (PSPIFFS_PAGE_OBJECT_IX)pfs->pucWorkBuffer;

    while (uiCurOffset < uiOffset + uiLen) {
        //TODO: SPIFFS_IX_MAP
    // #if SPIFFS_IX_MAP
    // // check if we have a memory, index map and if so, if we're within index map'pStat range
    // // and if so, if the entry is populated
    // if (pFd->ix_map && spanIXObjData >= pFd->ix_map->start_spix && spanIXObjData <= pFd->ix_map->end_spix
    //     && pFd->ix_map->map_buf[spanIXObjData - pFd->ix_map->start_spix]) {
    //     pageIXObjData = pFd->ix_map->map_buf[spanIXObjData - pFd->ix_map->start_spix];
    // } else {
    // #endif
        spanIXObjIXCur = SPIFFS_OBJ_IX_ENTRY_SPAN_IX(pfs, spanIXObjData);
        if (spanIXObjIXPrev != spanIXObjIXCur) {
            // load current object index (header) page
            if (spanIXObjIXCur == 0) {
                pageIXObjIX = pFd->pageIXObjIXHdr;
            } 
            else {
                SPIFFS_DBG("read: find objIX "_SPIPRIid":"_SPIPRIsp"\n", 
                            pFd->objId, spanIXObjIXCur);
                if (pFd->spanIXObjIXCursor == spanIXObjIXCur) {
                    pageIXObjIX = pFd->pageIXObjIXCursor;
                } 
                else {
                    iRes = spiffsObjLookUpFindIdAndSpan(pfs, pFd->objId | SPIFFS_OBJ_ID_IX_FLAG, spanIXObjIXCur, 
                                                        0, &pageIXObjIX);
                    SPIFFS_CHECK_RES(iRes);
                }
            }
            SPIFFS_DBG("read: load objIX page "_SPIPRIpg":"_SPIPRIsp" for data spix:"_SPIPRIsp"\n", pageIXObjIX, spanIXObjIXCur, spanIXObjData);
            iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_READ, pFd->fileN, 
                                   SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjIX), 
                                   SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), pfs->pucWorkBuffer);
            SPIFFS_CHECK_RES(iRes);
            SPIFFS_VALIDATE_OBJIX(objIX->pageHdr, pFd->objId, spanIXObjIXCur);

            pFd->uiOffset = uiCurOffset;
            pFd->pageIXObjIXCursor = pageIXObjIX;
            pFd->spanIXObjIXCursor = spanIXObjIXCur;

            spanIXObjIXPrev = spanIXObjIXCur;
        }

        /* ��ȡ����ҳ������� */
        if (spanIXObjIXCur == 0) {
            // get data page from object index header page
            /* Index Header�е����ݾ���һ������ҳ�� */
            pageIXObjData = ((SPIFFS_PAGE_IX*)((PCHAR)objIXHdr + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)))[spanIXObjData];
        } else {
            // get data page from object index page
            pageIXObjData = ((SPIFFS_PAGE_IX*)((PCHAR)objIX + sizeof(SPIFFS_PAGE_OBJECT_IX)))[SPIFFS_OBJ_IX_ENTRY(pfs, spanIXObjData)];
        }
    // #if SPIFFS_IX_MAP
    // }
    // #endif
        // all remaining data
        UINT32 uiByteToRead = uiOffset + uiLen - uiCurOffset;
        // remaining data in page
        uiByteToRead = MIN(uiByteToRead, SPIFFS_DATA_PAGE_SIZE(pfs) - (uiCurOffset % SPIFFS_DATA_PAGE_SIZE(pfs)));
        // remaining data in file
        uiByteToRead = MIN(uiByteToRead, pFd->uiSize - uiCurOffset);
        SPIFFS_DBG("read: uiOffset:"_SPIPRIi" rd:"_SPIPRIi" data spix:"_SPIPRIsp" is pageIXObjData:"_SPIPRIpg" addr:"_SPIPRIad"\n", 
                    uiCurOffset, uiByteToRead, spanIXObjData, pageIXObjData, (UINT32)(SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjData) + sizeof(SPIFFS_PAGE_HEADER) + (uiCurOffset % SPIFFS_DATA_PAGE_SIZE(pfs))));
        if (uiByteToRead <= 0) {
            iRes = SPIFFS_ERR_END_OF_OBJECT;
            break;
        }
        iRes = spiffsPageDataCheck(pfs, pFd, pageIXObjData, spanIXObjData);
        SPIFFS_CHECK_RES(iRes);
        
        iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_READ, pFd->fileN, 
                               (SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjData) + 
                               sizeof(SPIFFS_PAGE_HEADER) + 
                               (uiCurOffset % SPIFFS_DATA_PAGE_SIZE(pfs))),             /* ��ȡƫ�Ƶ�ַ */
                               uiByteToRead,
                               puDst);

        SPIFFS_CHECK_RES(iRes);
        puDst += uiByteToRead;
        uiCurOffset += uiByteToRead;
        pFd->uiOffset = uiCurOffset;
        spanIXObjData++;
    }

    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsFileWrite
** ��������: ��һ���ļ���д��
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsFileWrite(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler, PCHAR pcContent, 
                      UINT32 uiOffset, INT32 iLen){
    INT32       iRes = SPIFFS_OK;
    INT32       iRemaining = iLen;
    PSPIFFS_FD  pFd;
    PCHAR      pData = pcContent; 
    
    spiffsFdGet(pfs, fileHandler, &pFd);            /* ��ȡ�ļ������� */
    
    if (pFd->uiSize != SPIFFS_UNDEFINED_LEN && 
        uiOffset < pFd->uiSize) {                   /* �����޸�һ���� */
        INT32 iMinLen = MIN((INT32)(pFd->uiSize - uiOffset), iLen);
        iRes = spiffsObjectModify(pFd, uiOffset, pData, iMinLen);
        SPIFFS_CHECK_RES(iRes);
        iRemaining -= iMinLen;
        pData += iMinLen;
        uiOffset += iMinLen;
    }
    if (iRemaining > 0) {                           /* ʣ�µ�׷��д���� */
        iRes = spiffsObjectAppend(pFd, uiOffset, pData, iRemaining);
        SPIFFS_CHECK_RES(iRes);
    }
    return iLen;
}
/*********************************************************************************************************
** ��������: spiffsObjectTruncate
** ��������: ���ڽض�һ��Object
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsFileRead(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler, PCHAR pcContent, INT32 iLen) {
    //SPIFFS_API_CHECK_CFG(pfs);
    SPIFFS_API_CHECK_MOUNT(pfs);
    //SPIFFS_LOCK(pfs);

    PSPIFFS_FD pFd;
    INT32 iRes;

    iRes = spiffsFdGet(pfs, fileHandler, &pFd);     /* ����FileHander��ȡFd */

    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_CHECK_RES(iRes);

    if ((pFd->flags & SPIFFS_O_RDONLY) == 0) {
        iRes = SPIFFS_ERR_NOT_READABLE;
        //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
        SPIFFS_CHECK_RES(iRes);
    }

    if (pFd->uiSize == SPIFFS_UNDEFINED_LEN 
        && iLen > 0) {
        // special case for zero sized files
        iRes = SPIFFS_ERR_END_OF_OBJECT;
        //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
        SPIFFS_CHECK_RES(iRes);
    }

    /* ��Cache�������ˢ��Flash����֤д���һ���� */
    spiffsCacheFflush(pfs, fileHandler);


    if (pFd->uiFdOffset + iLen >= pFd->uiSize) {        /* ��ǰ�ļ���ƫ�� + ���� >= �ļ���С */
        // reading beyond file uiSize
        INT32 iAvail = pFd->uiSize - pFd->uiFdOffset;   /* ֻ���Զ�iAvail��ô�� */
        if (iAvail <= 0) {
            //SPIFFS_API_CHECK_RES_UNLOCK(pfs, SPIFFS_ERR_END_OF_OBJECT);
            iRes = SPIFFS_ERR_END_OF_OBJECT;
            SPIFFS_CHECK_RES(iRes);
        }
        iRes = spiffsObjectRead(pFd, pFd->uiFdOffset, iAvail, pcContent);
        if (iRes == SPIFFS_ERR_END_OF_OBJECT) {     /* �������� */
            pFd->uiFdOffset += iAvail;
            //SPIFFS_UNLOCK(pfs);
            return iAvail;
        } 
        else {
            //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
            SPIFFS_CHECK_RES(iRes);
            iLen = iAvail;
        }
    } 
    else {
        /* ���Զ��꣬GG */
        // reading within file uiSize
        iRes = spiffsObjectRead(pFd, pFd->uiFdOffset, iLen, pcContent);
        //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
        SPIFFS_CHECK_RES(iRes);
    }
    /* �����ļ�ָ�� */
    pFd->uiFdOffset += iLen;
    //SPIFFS_UNLOCK(pfs);

    return iLen;
}
/*********************************************************************************************************
** ��������: spiffsStatPageIX
** ��������: ��������ҳ��״̬
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsStatPageIX(PSPIFFS_VOLUME pfs, SPIFFS_PAGE_IX pageIX, SPIFFS_FILE fileHandler, PSPIFFS_STAT pStat) {
    (VOID)fileHandler;
    SPIFFS_PAGE_OBJECT_IX_HEADER objIXHdr;
    SPIFFS_OBJ_ID objId;
    /* ��Ҫ��֤pageIX��spanIXΪ0 */
    INT32 iRes = spiffsCacheRead(pfs,  SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_READ, fileHandler,
                                SPIFFS_PAGE_TO_PADDR(pfs, pageIX), 
                                sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER), (PCHAR)&objIXHdr);
    SPIFFS_API_CHECK_RES(pfs, iRes);
    //TODO:Ϊɶ��Ҫ����ObjID��Ӧ�ĵ�ַ�أ�objIXHdr.pageHdr.objId���ǿ���ֱ�ӻ�ȡ��
    UINT32 uiObjIdAddr = SPIFFS_BLOCK_TO_PADDR(pfs, SPIFFS_BLOCK_FOR_PAGE(pfs , pageIX)) +
                                               SPIFFS_OBJ_LOOKUP_ENTRY_FOR_PAGE(pfs, pageIX) * sizeof(SPIFFS_OBJ_ID);
    iRes = spiffsCacheRead(pfs,  SPIFFS_OP_T_OBJ_LU | SPIFFS_OP_C_READ, fileHandler,
                           uiObjIdAddr, sizeof(SPIFFS_OBJ_ID), 
                           (PCHAR)&objId);
    SPIFFS_API_CHECK_RES(pfs, iRes);

    pStat->objId = objId & ~SPIFFS_OBJ_ID_IX_FLAG;          /* ȡ�������� */
    pStat->objType = objIXHdr.type;
    pStat->uiSize = objIXHdr.uiSize == SPIFFS_UNDEFINED_LEN ? 0 : objIXHdr.uiSize;
    pStat->pageIX = pageIX;
    strncpy((PCHAR)pStat->ucName, (PCHAR)objIXHdr.ucName, SPIFFS_OBJ_NAME_LEN);

    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsTranslateToSylixOSFlag
** ��������: ת��Flag
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
SPIFFS_FLAGS spiffsTranslateToSylixOSFlag(INT iFlag){
    SPIFFS_FLAGS flags = 0;
    if((iFlag & O_CREAT) == O_CREAT){
        flags |= SPIFFS_O_CREAT;
    }
    if((iFlag & O_APPEND) == O_APPEND){
        flags |= SPIFFS_O_APPEND;
    }
    if((iFlag & O_TRUNC) == O_TRUNC){
        flags |= SPIFFS_O_TRUNC;
    }
    if((iFlag & O_EXCL) == O_EXCL){
        flags |= SPIFFS_O_EXCL;
    }
    if((iFlag & O_RDONLY) == O_RDONLY){       
        flags |= SPIFFS_O_RDONLY;
    }
    if((iFlag & O_WRONLY) == O_WRONLY){
        flags |= SPIFFS_O_WRONLY;
    }
    if((iFlag & O_RDWR) == O_RDWR){
        flags |= SPIFFS_O_RDWR;
    }
    return flags;
}
/*********************************************************************************************************
** ��������: spiffsDirRead
** ��������: ��������ҳ��״̬
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PSPIFFS_DIRENT spiffsDirRead(PSPIFFS_DIR pDir, PSPIFFS_DIRENT pDirent){
    SPIFFS_BLOCK_IX blkIX;
    INT             iEntry;
    INT32           iRes;

    iRes =  spiffsObjLookUpFindEntryVisitor(pDir->pfs, pDir->blkIX, pDir->uiEntry,
                                            SPIFFS_VIS_NO_WRAP,
                                            0,
                                            __spiffsReadDirVisitor,
                                            0,
                                            pDirent,
                                            &blkIX,
                                            &iEntry);
    if (iRes == SPIFFS_OK) {
        pDir->blkIX = blkIX;
        pDir->uiEntry = iEntry + 1;
        pDirent->objId &= ~SPIFFS_OBJ_ID_IX_FLAG;
        return pDirent;
    } 
    else {
        pDir->pfs->uiErrorCode = iRes;
    }

    return LW_NULL;
}
