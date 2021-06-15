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
       (pageHeader.flags & SPIFFS_PH_FLAG_IXDELE) == 0 && pageHeader.spanIX == 0) &&    /* ��ɾ���� */
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
    const PUCHAR pucConflictingName;
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
        SPIFFS_DBG("fs full\n");
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
** ��������: spiffsObjLookUpFindFreeObjId
** ��������: ����Ѱ��Free Obj ID
** �䡡��  : pfs          �ļ�ͷ
**           pObjId        ���ص�Object ID
**           pucConflictingName �ļ�·����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsObjectCreate(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId,
                         const UCHAR ucPath[], SPIFFS_OBJ_TYPE type, SPIFFS_PAGE_IX* pPageIXObjIndexHdr)
{
    INT32 iRes = SPIFFS_OK;
    SPIFFS_BLOCK_IX blkIX;
    SPIFFS_PAGE_OBJECT_IX_HEADER  pageObjIXHdr;
    INT iEntry;

    
}
