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
                           sizeof(SPIFFS_PAGE_HEADER), (PUCHAR)&pageHeader);
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
        if(iLookUpEntryIX == 0){        /* һ�������objIDΪ�գ�Ĭ�ϸÿ�Ϊ�գ������⣿��ʵ�Ϻܼ򵥣�LFSʹȻ*/
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
    PUCHAR pucConflictingName;
    UINT32 bitIX;
    UINT32 byteIX;
    SPIFFS_PAGE_IX pageIX;
    SPIFFS_PAGE_OBJECT_IX_HEADER objIXHdr;
    INT32 iRes = SPIFFS_OK;

    if(objId != SPIFFS_OBJ_ID_FREE && objId != SPIFFS_OBJ_ID_DELETED){
       objIdMin = *((SPIFFS_OBJ_ID*)pUserVar);
        pucConflictingName = (const PUCHAR)pUserConst;
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
    PUCHAR pucObjBitMap;
    UINT   uiIX;

    if(objId != SPIFFS_OBJ_ID_FREE && objId != SPIFFS_OBJ_ID_DELETED){
        pageIX = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iLookUpEntryIX);
        iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_READ, 0, 
                               SPIFFS_PAGE_TO_PADDR(pfs, pageIX), 
                               sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER), &objIXHdr);
        SPIFFS_CHECK_RES(iRes);
        if(objIXHdr.pageHdr.spanIX == 0 &&
           (objIXHdr.pageHdr.flags & (SPIFFS_PH_FLAG_DELET | SPIFFS_PH_FLAG_FINAL | SPIFFS_PH_FLAG_IXDELE)) 
           == (SPIFFS_PH_FLAG_DELET)){
            if(pState->pucConflictingName &&
               lib_strcmp((const PCHAR)pUserConst, (PCHAR)objIXHdr.ucName) == 0){       /* �ļ�����ͻ */
                return SPIFFS_ERR_CONFLICTING_NAME;
            }

            objId &= ~SPIFFS_OBJ_ID_IX_FLAG;
            if(objId >= pState->objIdMin && objId <= pState->objIdMax){
                pucObjBitMap = (PUCHAR)(pfs->pucWorkBuffer);
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
                           ,(PUCHAR)&objIXHdr);
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
/*********************************************************************************************************
** ��������: spiffsObjLookUpFindEntryVisitor
** ��������: ��Vistor������ÿ��Entry�����ʵ�objIDʱ�ᷢ���ص���flagsҲ��ص��й�
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
                                   uiBlkCurAddr + SPIFFS_PAGE_TO_PADDR(pfs, pageIXOffset), 
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
                                                       uiBlkCurAddr + SPIFFS_PAGE_TO_PADDR(pfs, pageIXOffset), 
                                                       SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), pfs->pucLookupWorkBuffer);
                                SPIFFS_CHECK_RES(iRes);
                            }
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
        iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU | SPIFFS_OP_C_READ, 0, 
                               SPIFFS_MAGIC_PADDR(pfs, blkIX), sizeof(SPIFFS_OBJ_ID), 
                               (PUCHAR)&objIdMagic);
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

        iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_READ, 0, 
                               SPIFFS_ERASE_COUNT_PADDR(pfs, blkIX), sizeof(SPIFFS_OBJ_ID),
                               (PUCHAR)&objIdEraseCount);
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
INT32 spiffsObjLookUpFindFreeObjId(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID *pObjId, const PUCHAR pucConflictingName){
    INT32                    iRes         = SPIFFS_OK;
    UINT32                   uiMaxObjects = (pfs->uiBlkCount * SPIFFS_OBJ_LOOKUP_MAX_ENTRIES(pfs)) / 2;  /* ���Objects������Ϊһ��Object������һ��Index��һ��Data���� */
    SPIFFS_OBJ_ID            objIdFree    = SPIFFS_OBJ_ID_FREE;
    SPIFFS_FREE_OBJ_ID_STATE state;
    UINT                     i,j;
    
    UINT32                   uiMinIndex = 0;
    UINT8                    uiMinCount = (UINT8)-1;
    PUCHAR                   pucBitMap;

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
                for (j = 0; i < 8; i++)
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
                
                SPIFFS_DBG("free_obj_id: COMP select index:"_SPIPRIi" min_count:"_SPIPRIi" min:"_SPIPRIid" max:"_SPIPRIid" compact:"_SPIPRIi"\n", uiMinIndex, uiMinCount, state.objIdMin, state.objIdMax, state.uiCompaction);
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
            iRes = spiffsObjLookUpFindEntryVisitor(pfs,0, 0, 0, 0, __spiffsObjLookUpFindFreeObjIdCompactVistor, 
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

INT32 spiffsObjLookUpFindIdAndSpan(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, SPIFFS_SPAN_IX spanIX,
                                   SPIFFS_PAGE_IX pageIXExclusion, SPIFFS_PAGE_IX *pPageIX){
    INT32 iRes;
    SPIFFS_BLOCK_IX blkIX;
    INT iEntry;

    iRes = spiffsObjLookUpFindEntryVisitor(pfs, pfs->blkIXCursor, pfs->objLookupEntryCursor, SPIFFS_VIS_CHECK_ID, objId,
                                           __spiffsObjLookUpFindIdAndSpanVistor, pageIXExclusion ? &pageIXExclusion : LW_NULL,
                                           &spanIX, &blkIX, &iEntry);
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
                           (PUCHAR)&pageHeader);
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
                           sizeof(SPIFFS_PAGE_HEADER),(PUCHAR)&pageHeader);
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
                            sizeof(SPIFFS_OBJ_ID), (PUCHAR)&d_obj_id);
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
                             PUCHAR pData, UINT32 uiLen, UINT32 uiPageOffs, BOOL bIsFinalize,
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
    /* �õ�ǰ����ҳ���objIDռ�����entry */
    iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_LU | SPIFFS_OP_C_UPDT, 0, 
                            SPIFFS_BLOCK_TO_PADDR(pfs, blkIX) + iEntry * sizeof(SPIFFS_OBJ_ID), 
                            sizeof(SPIFFS_OBJ_ID), (PUCHAR)&objId);
    SPIFFS_CHECK_RES(iRes);

    pfs->uiStatsPageAllocated++;

    // write page header
    pPageHeader->flags &= ~SPIFFS_PH_FLAG_USED;
    iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_UPDT, 0, 
                            SPIFFS_OBJ_LOOKUP_ENTRY_TO_PADDR(pfs, blkIX, iEntry), 
                            sizeof(SPIFFS_PAGE_HEADER), (PUCHAR)pPageHeader);
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
                                sizeof(UINT8),(PUCHAR)&pPageHeader->flags);
        SPIFFS_CHECK_RES(iRes);
    }

    // return written page
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
                                 PUCHAR pucNewObjIXHdrData , const UCHAR ucName[], UINT32 uiSize, SPIFFS_PAGE_IX *pageIXNew){
    INT32 iRes = SPIFFS_OK;
    PSPIFFS_PAGE_OBJECT_IX_HEADER objIXHdr;
    SPIFFS_PAGE_IX pageIXObjIXHdrNew;

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
                               SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjIXHdr), SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), pfs->pucWorkBuffer);
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
    //TODO: ΪʲôҪ�ƶ�ҳ��
    iRes = spiffsPageMove(pfs, pFd == LW_NULL ? LW_NULL : pFd->fileN, 
                          (PUCHAR)objIXHdr, objId, 0, pageIXObjIXHdr, &pageIXObjIXHdrNew);

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
                            sizeof(SPIFFS_OBJ_ID), (PUCHAR)&objId);
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
                            sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER), (PUCHAR)&objIXHdr);

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
                           sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER), (PUCHAR)&objIXHdr);
    SPIFFS_CHECK_RES(iRes);


    iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU | SPIFFS_OP_C_READ, 0, 
                           SPIFFS_BLOCK_TO_PADDR(pfs, blkIX) + iEntry * sizeof(SPIFFS_OBJ_ID), 
                           sizeof(SPIFFS_OBJ_ID), (PUCHAR)&objId);

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
    SPIFFS_SPAN_IX                  spanIXData = (pFd->uiSize > 0 ? pFd->uiSize - 1 : 0) / SPIFFS_DATA_PAGE_SIZE(pfs);
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
        spanIXObjIXCur = SPIFFS_OBJ_IX_ENTRY_SPAN_IX(pfs, spanIXData);      /* ��ǰ SpanIX Data ��Ӧ���Ǹ�Object IX��Span */
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
                       pageIXObjIX, spanIXObjIXCur, spanIXData);
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
            pageIXData = ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIXHdr + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)))[spanIXData];
            ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIXHdr + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)))[spanIXData] = SPIFFS_OBJ_ID_FREE;
        } 
        else {
            // get data page from object index page
            pageIXData = ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIX + sizeof(SPIFFS_PAGE_OBJECT_IX)))[SPIFFS_OBJ_IX_ENTRY(pfs, spanIXData)];
            ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIX + sizeof(SPIFFS_PAGE_OBJECT_IX)))[SPIFFS_OBJ_IX_ENTRY(pfs, spanIXData)] = SPIFFS_OBJ_ID_FREE;
        }

        SPIFFS_DBG("truncate: got data pageIX "_SPIPRIpg"\n", pageIXData);

        if (uiNewSize == 0 || bIsRemoveFull 
         || uiCurSize - uiNewSize >= SPIFFS_DATA_PAGE_SIZE(pfs)) {
            // delete full data page
            //TODO��Ϊɶ���ﻹҪ��ɾ���أ�ǰ�治���Ѿ�ɾ����
            /* ����ɾ��һ����ҳ�� */
            iRes = spiffsPageDataCheck(pfs, pFd, pageIXData, spanIXData);
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
                       pageIXData, spanIXData, uiCurSize);
        } 
        else {
            // delete last page, partially
            /* ���һ��ҳ�治һ��ȫ��ɾ�� */
            SPIFFS_PAGE_HEADER pageHeader;
            SPIFFS_PAGE_IX pageIXDataNew;
            UINT32 uiBytesToRemove = SPIFFS_DATA_PAGE_SIZE(pfs) - (uiNewSize % SPIFFS_DATA_PAGE_SIZE(pfs));
            
            SPIFFS_DBG("truncate: delete "_SPIPRIi" bytes from data page "_SPIPRIpg" for data spix:"_SPIPRIsp", uiCurSize:"_SPIPRIi"\n", 
                       uiBytesToRemove, pageIXData, spanIXData, uiCurSize);

            iRes = spiffsPageDataCheck(pfs, pFd, pageIXData, spanIXData);
            if (iRes != SPIFFS_OK) 
                break;

            pageHeader.objId = pFd->objId & ~SPIFFS_OBJ_ID_IX_FLAG;
            pageHeader.spanIX = spanIXData;
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
                                    sizeof(UINT8), (PUCHAR)&pageHeader.flags);
            if (iRes != SPIFFS_OK) 
                break;

            // update memory representation of object index page with new data page
            /* �����ڴ� IX Obj */
            if (spanIXObjIXCur == 0) {
                /* �����IX OBJ�ĵ�0ҳ */
                // update object index header page
                ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIXHdr + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)))[spanIXData] = pageIXDataNew;
                SPIFFS_DBG("truncate: wrote page "_SPIPRIpg" to pObjIXHdr iEntry "_SPIPRIsp" in mem\n", 
                           pageIXDataNew, (SPIFFS_SPAN_IX)SPIFFS_OBJ_IX_ENTRY(pfs, spanIXData));
            } 
            else {
                /* �����IX OBJ�ķǵ�0ҳ */
                // update object index page
                ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIX + sizeof(SPIFFS_PAGE_OBJECT_IX)))[SPIFFS_OBJ_IX_ENTRY(pfs, spanIXData)] = pageIXDataNew;
                SPIFFS_DBG("truncate: wrote page "_SPIPRIpg" to pObjIX iEntry "_SPIPRIsp" in mem\n", pageIXDataNew, (SPIFFS_SPAN_IX)SPIFFS_OBJ_IX_ENTRY(pfs, spanIXData));
            }
            uiCurSize = uiNewSize;
            pFd->uiSize = uiNewSize;
            pFd->uiOffset = uiCurSize;
            break;
        }
        spanIXData--;
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
INT32 spiffsFileWrite(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler, PVOID pContent, 
                      UINT32 uiOffset, INT32 iLen){
    (VOID)pfs;
    INT32       iRes = SPIFFS_OK;
    INT32       iRemaining = iLen;
    PSPIFFS_FD  pFd;
    PUCHAR      pData = (PUCHAR)pContent; 
    
    spiffsFdGet(pfs, fileHandler, &pFd);
    
    if (pFd->uiSize != SPIFFS_UNDEFINED_LEN && 
        uiOffset < pFd->uiSize) {
        INT32 iMinLen = MIN((INT32)(pFd->uiSize - uiOffset), iLen);
        iRes = spiffsObjectModify(pFd, uiOffset, pData, iMinLen);
        SPIFFS_CHECK_RES(iRes);
        iRemaining -= iMinLen;
        pData += iMinLen;
        uiOffset += iMinLen;
    }
    if (iRemaining > 0) {
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
INT32 spiffsFileRead(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler, PVOID pContent, INT32 iLen) {
    //SPIFFS_API_CHECK_CFG(pfs);
    SPIFFS_API_CHECK_MOUNT(pfs);
    //SPIFFS_LOCK(pfs);

    PSPIFFS_FD pFd;
    INT32 iRes;

    fileHandler = fileHandler;
    iRes = spiffsFdGet(pfs, fileHandler, &pFd);

    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);

    if ((pFd->flags & SPIFFS_O_RDONLY) == 0) {
        iRes = SPIFFS_ERR_NOT_READABLE;
        //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    }

    if (pFd->uiSize == SPIFFS_UNDEFINED_LEN 
        && iLen > 0) {
        // special case for zero sized files
        iRes = SPIFFS_ERR_END_OF_OBJECT;
        //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    }

    spiffsCacheFflush(pfs, fileHandler);


    if (pFd->fdoffset + iLen >= pFd->uiSize) {
    // reading beyond file uiSize
    INT32 avail = pFd->uiSize - pFd->fdoffset;
    if (avail <= 0) {
        SPIFFS_API_CHECK_RES_UNLOCK(pfs, SPIFFS_ERR_END_OF_OBJECT);
    }
    iRes = spiffs_object_read(pFd, pFd->fdoffset, avail, (u8_t*)pContent);
    if (iRes == SPIFFS_ERR_END_OF_OBJECT) {
        pFd->fdoffset += avail;
        SPIFFS_UNLOCK(pfs);
        return avail;
    } else {
        SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
        iLen = avail;
    }
    } else {
    // reading within file uiSize
    iRes = spiffs_object_read(pFd, pFd->fdoffset, iLen, (u8_t*)pContent);
    SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    }
    pFd->fdoffset += iLen;

    SPIFFS_UNLOCK(pfs);

    return iLen;
}