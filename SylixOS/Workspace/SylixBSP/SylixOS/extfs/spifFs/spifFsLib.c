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
** 文   件   名: spifFsLib.c
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 01日
**
** 描        述: Spiffs文件系统核心
*********************************************************************************************************/
#include "spifFsLib.h"
#include "spifFsCache.h"
#include "spifFsGC.h"
#include "spifFsFDLib.h"

INT32 __spiffsObjLookUpFindIdAndSpanVistor(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, SPIFFS_BLOCK_IX blkIX, INT iLookUpEntryIX, 
                                           const PVOID pUserConst, PVOID pUserVar){
    INT32 iRes;
    SPIFFS_PAGE_HEADER pageHeader;
    SPIFFS_PAGE_IX pageIX = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iLookUpEntryIX);
    iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_READ, 0, 
                           SPIFFS_PAGE_TO_PADDR(pfs, pageIX), 
                           sizeof(SPIFFS_PAGE_HEADER), (PUCHAR)&pageHeader);
    SPIFFS_CHECK_RES(iRes);
    //TODO:什么意思？？？
    if(pageHeader.objId == objId &&
       pageHeader.spanIX == *((SPIFFS_SPAN_IX *)pUserVar) &&
       (pageHeader.flags & (SPIFFS_PH_FLAG_FINAL | SPIFFS_PH_FLAG_DELET | SPIFFS_PH_FLAG_USED)) == SPIFFS_PH_FLAG_DELET &&
       !((objId & SPIFFS_OBJ_ID_IX_FLAG) &&                                             /* 不是Obj IX */
       (pageHeader.flags & SPIFFS_PH_FLAG_IXDELE) == 0 && pageHeader.spanIX == 0) &&    /* 被删了吗？ */
       (pUserConst == LW_NULL || *((const SPIFFS_PAGE_IX *)pUserConst) != pageIX)){     
        return SPIFFS_OK;
    }
    else {
        return SPIFFS_VIS_COUNTINUE;
    }
}
/*********************************************************************************************************
** 函数名称: __spiffsObjLookUpScanVistor
** 功能描述: 扫描并统计Page信息的Vistor
** 输　入  : pfs          文件头
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 __spiffsObjLookUpScanVistor(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, SPIFFS_BLOCK_IX blkIX, INT iLookUpEntryIX, 
                                  const PVOID pUserConst, PVOID pUserVar){
    (VOID) blkIX;
    (VOID) pUserConst;
    (VOID) pUserVar;

    if(objId == SPIFFS_OBJ_ID_FREE){
        if(iLookUpEntryIX == 0){        /* 一个块的首objID为空，默认该块为空，如何理解？事实上很简单，LFS使然*/
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
** 函数名称: __spiffsObjLookUpFindFreeObjIdBitmapVistor
** 功能描述: 用位于WorkBuffer的内存收集有效块信息
** 输　入  : pfs          文件头
** 输　出  : None
** 全局变量:
** 调用模块:
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
        /* 找到一个标记为Index的Entry */
        if(pucConflictingName && (objId & SPIFFS_OBJ_ID_IX_FLAG)){
            pageIX = SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iLookUpEntryIX);
            iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_READ, 0, 
                                   SPIFFS_PAGE_TO_PADDR(pfs, pageIX), 
                                   sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER), &objIXHdr);
            SPIFFS_CHECK_RES(iRes);
            if(objIXHdr.pageHdr.spanIX == 0 &&
               (objIXHdr.pageHdr.flags & (SPIFFS_PH_FLAG_DELET | SPIFFS_PH_FLAG_FINAL | SPIFFS_PH_FLAG_IXDELE)) 
               == (SPIFFS_PH_FLAG_DELET | SPIFFS_PH_FLAG_IXDELE)){
                if(lib_strcmp((const PCHAR)pUserConst, (PCHAR)objIXHdr.ucName) == 0){       /* 文件名冲突 */
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
** 函数名称: __spiffsObjLookUpFindFreeObjIdCompactVistor
** 功能描述: 用位于WorkBuffer的内存收集有效块信息，重用压缩收集
** 输　入  : pfs          文件头
** 输　出  : None
** 全局变量:
** 调用模块:
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
               lib_strcmp((const PCHAR)pUserConst, (PCHAR)objIXHdr.ucName) == 0){       /* 文件名冲突 */
                return SPIFFS_ERR_CONFLICTING_NAME;
            }

            objId &= ~SPIFFS_OBJ_ID_IX_FLAG;
            if(objId >= pState->objIdMin && objId <= pState->objIdMax){
                pucObjBitMap = (PUCHAR)(pfs->pucWorkBuffer);
                /* (id - min) * page_sz  / (max - min)*/
                uiIX = (objId - pState->objIdMin) / pState->uiCompaction;   /* 重叠的第几个页 */
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
** 函数名称: spiffsObjLookUpFindEntryVisitor
** 功能描述: 用Vistor来访问每个Entry，访问到objID时会发生回调，flags也与回调有关
** 输　入  : pfs          文件头
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 spiffsObjLookUpFindEntryVisitor(PSPIFFS_VOLUME pfs, SPIFFS_BLOCK_IX blkIXStarting, INT iLookUpEntryStarting,
                                      UINT8 flags, SPIFFS_OBJ_ID objId, spiffsVisitorFunc vistor,
                                      const PVOID pUserConst, PVOID pUserVar, SPIFFS_BLOCK_IX *pBlkIX, INT *piLookUpEntry){
    INT32           iRes               = SPIFFS_OK;
    INT32           iEntryCount        = pfs->uiBlkCount * SPIFFS_OBJ_LOOKUP_MAX_ENTRIES(pfs);  /* 总共最多的Entry */
    SPIFFS_BLOCK_IX blkIXCur           = blkIXStarting;
    UINT32          uiBlkCurAddr       = SPIFFS_BLOCK_TO_PADDR(pfs, blkIXCur);
    SPIFFS_OBJ_ID   *pObjLookUpBuffer  = (SPIFFS_OBJ_ID *)pfs->pucLookupWorkBuffer;
    INT             iEntryCur          = iLookUpEntryStarting;
    INT             iEntriesPerPage    = SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) / sizeof(SPIFFS_OBJ_ID); /* LookUp Page一个Page可以有多少个Entry */
    
    SPIFFS_PAGE_IX  pageIXOffset;   /* 相对Blk LookUp页面，1 ~ SPIFFS_OBJ_LOOKUP_PAGES */
    INT             iEntryOffset;              

    /* 超出了一个块的最大Entry数 - 1，略过Erase Count，检查下一个块 */
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

    /* 遍历每个Blk */
    while (iRes == SPIFFS_OK && iEntryCount > 0)    
    {
        pageIXOffset = iEntryCur / iEntriesPerPage;  
        /* 查看每个LookUp Page */
        while (iRes == SPIFFS_OK && pageIXOffset < (INT)SPIFFS_OBJ_LOOKUP_PAGES(pfs))
        {
            iEntryOffset = pageIXOffset * iEntriesPerPage;
            iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU | SPIFFS_OP_C_READ, 0, 
                                   uiBlkCurAddr + SPIFFS_PAGE_TO_PADDR(pfs, pageIXOffset), 
                                   SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), pfs->pucLookupWorkBuffer);

            /* 查看LookUp Page上的所有Entry */
            while (iRes == SPIFFS_OK && 
                   iEntryCur < (INT)SPIFFS_OBJ_LOOKUP_MAX_ENTRIES(pfs) &&   /* 最后一个页面 */
                   iEntryCur - iEntryOffset < iEntriesPerPage)              /* 非最后一个页面 */
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
                        //TODO: 为啥不是返回 SPIFFS_VIS_END
                        return SPIFFS_OK;
                    }
                }
                iEntryCount--;
                iEntryCur++;
            } /* 结束遍历每个Look Up Page的Entry */
            pageIXOffset++;
        } /* 结束遍历每个Look Up Page */
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
    } /* 结束遍历每个Blk */
    
    SPIFFS_CHECK_RES(iRes);
    return SPIFFS_VIS_END;
}
/*********************************************************************************************************
** 函数名称: spiffsObjLookUpScan
** 功能描述: 扫描Flash介质上的所有Lookup Page，记录页面状态（Deleted、Used等），并找到最大块擦除次数
** 输　入  : pfs          文件头
** 输　出  : None
** 全局变量:
** 调用模块:
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
    
    /* 完成EraseCount的计数 */
    while (blkIX < pfs->uiBlkCount)
    {
        iRes = spiffsCacheRead(pfs, SPIFFS_OP_T_OBJ_LU | SPIFFS_OP_C_READ, 0, 
                               SPIFFS_MAGIC_PADDR(pfs, blkIX), sizeof(SPIFFS_OBJ_ID), 
                               (PUCHAR)&objIdMagic);
        SPIFFS_CHECK_RES(iRes);
        //TODO: 这里扫描的时候有些块未配置，也就是说必须要先format才能使用该文件系统嘛？
        if(objIdMagic != SPIFFS_MAGIC_PADDR(pfs, blkIX)){
            if(blkIXUnerased == SPIFFS_OBJ_ID_FREE){   /* 分配一个未擦写块，有可能是掉电了，Magic才不同 */
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

    //TODO: 搞清楚这个擦除次数是如何计算的
    /* 未经擦除的系统 */
    if(objIdEraseCountMin == 0 && objIdEraseCountMax == SPIFFS_OBJ_ID_FREE){
        objIdEraseCountFinal = 0;
    }
    //TODO: 最高位为1代表Index Page，这里是在做约束吗
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
** 函数名称: spiffsObjLookUpFindFreeObjId
** 功能描述: 用于寻找Free Obj ID
** 输　入  : pfs          文件头
**           pObjId        返回的Object ID
**           pucConflictingName 文件路径名
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 spiffsObjLookUpFindFreeObjId(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID *pObjId, const PUCHAR pucConflictingName){
    INT32                    iRes         = SPIFFS_OK;
    UINT32                   uiMaxObjects = (pfs->uiBlkCount * SPIFFS_OBJ_LOOKUP_MAX_ENTRIES(pfs)) / 2;  /* 最大Objects数，因为一个Object至少由一个Index和一个Data构成 */
    SPIFFS_OBJ_ID            objIdFree    = SPIFFS_OBJ_ID_FREE;
    SPIFFS_FREE_OBJ_ID_STATE state;
    UINT                     i,j;
    
    UINT32                   uiMinIndex = 0;
    UINT8                    uiMinCount = (UINT8)-1;
    PUCHAR                   pucBitMap;

    UINT8                    uiMask8;
    //TODO: State是用来干嘛的？
    //!用于打包
    state.objIdMin = 1;
    state.objIdMax = uiMaxObjects + 1;
    if(state.objIdMax & SPIFFS_OBJ_ID_IX_FLAG){
        state.objIdMax = ((SPIFFS_OBJ_ID)-1) & ~SPIFFS_OBJ_ID_IX_FLAG;
    }
    state.uiCompaction = 0;
    state.pucConflictingName = pucConflictingName;

    while (iRes == SPIFFS_OK && objIdFree == SPIFFS_OBJ_ID_FREE)
    {   
        /* 可以装在一个页面里，一个字节8位 */
        if(state.objIdMax - state.objIdMin 
           <= (SPIFFS_OBJ_ID)SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) * 8){
            /* 可以放在一个bitmap里 */
            SPIFFS_DBG("free_obj_id: BITMap min:"_SPIPRIid" max:"_SPIPRIid"\n", state.objIdMin, state.objIdMax);
            lib_memset(pfs->pucWorkBuffer, 0, SPIFFS_CFG_LOGIC_PAGE_SZ(pfs));   /* 清空work，用于记录位图数据 */
            iRes = spiffsObjLookUpFindEntryVisitor(pfs, 0, 0, 0, 0, __spiffsObjLookUpFindFreeObjIdBitmapVistor,
                                                   pucConflictingName, &state.objIdMin, LW_NULL, LW_NULL);
            if(iRes == SPIFFS_VIS_END){
                iRes = SPIFFS_OK;
            }
            SPIFFS_CHECK_RES(iRes);

            /* 遍历每个字节 */
            for (i = 0; i < SPIFFS_CFG_LOGIC_PAGE_SZ(pfs); i++)
            {
                uiMask8 = pfs->pucWorkBuffer[i];
                if(uiMask8 == (UINT8)-1){
                    continue;
                }
                /* 遍历每个位 */
                /* 8位掩码 */
                for (j = 0; i < 8; i++)
                {
                    if((uiMask8 & (1 << j)) == 0){
                        *pObjId = state.objIdMin + j + (i << 3);
                        return SPIFFS_OK;
                    }
                }
                
            }
            return SPIFFS_ERR_FULL;     /* 没有空余的Obj了 */           
        }
        /* 不能装在一个页面里，不过可以多次重叠，最后找小的，你懂的 */
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

                if(uiMinCount == state.uiCompaction){       /* 没有空闲OBJ了 */
                    SPIFFS_DBG("free_obj_id: compacted table is full\n");
                    return SPIFFS_ERR_FULL;
                }
                
                SPIFFS_DBG("free_obj_id: COMP select index:"_SPIPRIi" min_count:"_SPIPRIi" min:"_SPIPRIid" max:"_SPIPRIid" compact:"_SPIPRIi"\n", uiMinIndex, uiMinCount, state.objIdMin, state.objIdMax, state.uiCompaction);
                if(uiMinCount == 0){
                    *pObjId = uiMinIndex * state.uiCompaction + state.objIdMin;
                    return SPIFFS_OK;
                }
                else {
                    /* 减少压缩范围 */
                    SPIFFS_DBG("free_obj_id: COMP SEL chunk:"_SPIPRIi" min:"_SPIPRIid" -> "_SPIPRIid"\n", state.uiCompaction, state.objIdMin, state.objIdMin + uiMinIndex *  state.uiCompaction);
                    state.objIdMin += (uiMinIndex * state.uiCompaction);
                    state.objIdMax = state.objIdMin + state.uiCompaction;
                }

                if(state.objIdMax - state.objIdMin 
                    <= (SPIFFS_OBJ_ID)SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) * 8){
                    /* 可以用一页装了，那就直接装，不用压缩了 */
                    continue;
                }
            }
            state.uiCompaction = (state.objIdMax - state.objIdMin) / ((SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) / sizeof(UINT8))); /* 页面总数 */
            SPIFFS_DBG("free_obj_id: COMP min:"_SPIPRIid" max:"_SPIPRIid" compact:"_SPIPRIi"\n", state.objIdMin, 
                       state.objIdMax, state.uiCompaction);
            lib_memset(pfs->pucWorkBuffer, 0, SPIFFS_CFG_LOGIC_PAGE_SZ(pfs));
            iRes = spiffsObjLookUpFindEntryVisitor(pfs,0, 0, 0, 0, __spiffsObjLookUpFindFreeObjIdCompactVistor, 
                                                   &state, LW_NULL, LW_NULL, LW_NULL);
            if(iRes == SPIFFS_VIS_END){
                iRes = SPIFFS_OK;
            }
            SPIFFS_CHECK_RES(iRes);
            state.pucConflictingName = LW_NULL; /* 找一次就够了 */
        }
    }
}
/*********************************************************************************************************
** 函数名称: spiffsObjLookUpFindId
** 功能描述: 用于寻找给定的ObjId
** 输　入  : pfs          文件头
**           pObjId        返回的Object ID
**           pucConflictingName 文件路径名
** 输　出  : None
** 全局变量:
** 调用模块:
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
** 函数名称: spiffsObjLookUpFindFreeEntry
** 功能描述: 用于寻找Free Look Up Entry
** 输　入  : pfs          文件头
**           pObjId        返回的Object ID
**           pucConflictingName 文件路径名
** 输　出  : None
** 全局变量:
** 调用模块:
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
    /* 找到一个Id为111的Entry */
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
** 函数名称: spiffsObjectFindObjectIndexHeaderByName
** 功能描述: 用于寻找Free Obj ID
** 输　入  : pfs          文件头
**           pObjId        返回的Object ID
**           pucConflictingName 文件路径名
** 输　出  : None
** 全局变量:
** 调用模块:
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
** 函数名称: spiffsObjectUpdateIndexHdr
** 功能描述: 更新索引页头
** 输　入  : pfs          文件头
**           pObjId        返回的Object ID
**           pucConflictingName 文件路径名
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 spiffsObjectUpdateIndexHdr(PSPIFFS_VOLUME pfs, PSPIFFS_FD pFd, SPIFFS_OBJ_ID objId, SPIFFS_PAGE_IX pageIXObjIXHdr,
                                 PUCHAR pucNewObjIXHdrData , const UCHAR ucName[], UINT32 uiSize, SPIFFS_PAGE_IX *pageIXNew){
    INT32 iRes = SPIFFS_OK;
    PSPIFFS_PAGE_OBJECT_IX_HEADER objIXHdr;
    SPIFFS_PAGE_IX pageIXObjIXHdrNew;

    objId |=  SPIFFS_OBJ_ID_IX_FLAG;

    /* 获取Index页面的Hdr */
    if (pucNewObjIXHdrData) {
        // object index header page already given to us, no need to load it
        objIXHdr = (PSPIFFS_PAGE_OBJECT_IX_HEADER)pucNewObjIXHdrData;
    } else {
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
** 函数名称: spiffsObjectCreate
** 功能描述: 用于创建一个Object
** 输　入  : pfs          文件头
**           pObjId        返回的Object ID
**           pucConflictingName 文件路径名
** 输　出  : None
** 全局变量:
** 调用模块:
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

    objId |= SPIFFS_OBJ_ID_IX_FLAG;             /* 转换成该ID对应的目录 */
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
** 函数名称: spiffsObjectOpenByPage
** 功能描述: 根据一个pageIX打开一个文件
** 输　入  : pfs          文件头
**           pageIX        返回的Object ID
**           pFd           空闲的Fd
**           flags         打开方式
**           mode          目的在于与Posix兼容
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 spiffsObjectOpenByPage(PSPIFFS_VOLUME pfs, SPIFFS_PAGE_IX pageIX, 
                             PSPIFFS_FD pFd, SPIFFS_FLAGS flags, SPIFFS_MODE mode){
    //TODO: 该页面是Index页吗？
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
** 函数名称: spiffsObjectTruncate
** 功能描述: 用于截断一个Object
** 输　入  : pfs          文件头
**           pObjId        返回的Object ID
**           pucConflictingName 文件路径名
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 spiffsObjectTruncate(PSPIFFS_FD pFd, UINT32 uiNewSize, BOOL bIsRemoveFull){
    INT32          iRes = SPIFFS_OK;
    PSPIFFS_VOLUME pfs = pFd->pfs;

    /* 不用干任何事情 */
    if ((pFd->uiSize == SPIFFS_UNDEFINED_LEN || pFd->uiSize == 0) && !bIsRemoveFull) {
        // no op
        return iRes;
    }

    // need 2 pages if not removing: object index page + possibly chopped data page
    if (bIsRemoveFull == LW_FALSE) {
        iRes = spiffsGCCheck(pfs, SPIFFS_DATA_PAGE_SIZE(pfs) * 2);
        SPIFFS_CHECK_RES(iRes);
    }

    SPIFFS_PAGE_IX                  pageIXObjIX = pFd->pageIXObjIXHdr;  /* 索引Obj的页码 */
    SPIFFS_SPAN_IX                  spanIXData = (pFd->uiSize > 0 ? pFd->uiSize - 1 : 0) / SPIFFS_DATA_PAGE_SIZE(pfs);
    UINT32                          uiCurSize = pFd->uiSize == (UINT32)SPIFFS_UNDEFINED_LEN ? 0 : pFd->uiSize;
    SPIFFS_SPAN_IX                  spanIXObjIXCur = 0;
    SPIFFS_SPAN_IX                  spanIXObjIXPrev = (SPIFFS_SPAN_IX)-1;
    PSPIFFS_PAGE_OBJECT_IX_HEADER   pObjIXHdr = (SPIFFS_PAGE_OBJECT_IX_HEADER *)pfs->pucWorkBuffer;
    SPIFFS_PAGE_OBJECT_IX           *pObjIX = (SPIFFS_PAGE_OBJECT_IX *)pfs->pucWorkBuffer;
    SPIFFS_PAGE_IX                  pageIXData;
    SPIFFS_PAGE_IX                  newPageIXObjIXHdr;

    // before truncating, check if object is to be fully removed and mark this
    if (bIsRemoveFull && uiNewSize == 0) {
    UINT8 flags = ~( SPIFFS_PH_FLAG_USED | SPIFFS_PH_FLAG_INDEX | SPIFFS_PH_FLAG_FINAL | SPIFFS_PH_FLAG_IXDELE);
    iRes = _spiffs_wr(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_UPDT,
        pFd->fileN, SPIFFS_PAGE_TO_PADDR(pfs, pFd->pageIXObjIXHdr) + offsetof(SPIFFS_PAGE_HEADER, flags),
        sizeof(UINT8),
        (UINT8 *)&flags);
    SPIFFS_CHECK_RES(iRes);
    }

    // delete from end of object until desired len is reached
    while (uiCurSize > uiNewSize) {
    spanIXObjIXCur = SPIFFS_OBJ_IX_ENTRY_SPAN_IX(pfs, spanIXData);

    // put object index for current data span index in pucWorkBuffer buffer
    if (spanIXObjIXPrev != spanIXObjIXCur) {
        if (spanIXObjIXPrev != (SPIFFS_SPAN_IX)-1) {
        // remove previous object index page
        SPIFFS_DBG("truncate: delete pObjIX page "_SPIPRIpg":"_SPIPRIsp"\n", pageIXObjIX, spanIXObjIXPrev);

        iRes = spiffs_page_index_check(pfs, pFd, pageIXObjIX, spanIXObjIXPrev);
        SPIFFS_CHECK_RES(iRes);

        iRes = spiffs_page_delete(pfs, pageIXObjIX);
        SPIFFS_CHECK_RES(iRes);
        spiffs_cb_object_event(pfs, (SPIFFS_PAGE_OBJECT_IX *)0,
            SPIFFS_EV_IX_DEL, pFd->objId, pObjIX->p_hdr.span_ix, pageIXObjIX, 0);
        if (spanIXObjIXPrev > 0) {
            // Update object index header page, unless we totally want to remove the file.
            // If fully removing, we're not keeping consistency as good as when storing the header between chunks,
            // would we be aborted. But when removing full files, a crammed system may otherwise
            // report ERR_FULL a la windows. We cannot have that.
            // Hence, take the risk - if aborted, a file check would free the lost pages and mend things
            // as the file is marked as fully deleted in the beginning.
            if (bIsRemoveFull == 0) {
            SPIFFS_DBG("truncate: update pObjIX hdr page "_SPIPRIpg":"_SPIPRIsp" to uiSize "_SPIPRIi"\n", pFd->pageIXObjIXHdr, spanIXObjIXPrev, uiCurSize);
            iRes = spiffs_object_update_index_hdr(pfs, pFd, pFd->objId,
                pFd->pageIXObjIXHdr, 0, 0, 0, uiCurSize, &newPageIXObjIXHdr);
            SPIFFS_CHECK_RES(iRes);
            }
            pFd->uiSize = uiCurSize;
        }
        }
        // load current object index (header) page
        if (spanIXObjIXCur == 0) {
        pageIXObjIX = pFd->pageIXObjIXHdr;
        } else {
        iRes = spiffs_obj_lu_find_id_and_span(pfs, pFd->objId | SPIFFS_OBJ_ID_IX_FLAG, spanIXObjIXCur, 0, &pageIXObjIX);
        SPIFFS_CHECK_RES(iRes);
        }

        SPIFFS_DBG("truncate: load pObjIX page "_SPIPRIpg":"_SPIPRIsp" for data spix:"_SPIPRIsp"\n", pageIXObjIX, spanIXObjIXCur, spanIXData);
        iRes = _spiffs_rd(pfs, SPIFFS_OP_T_OBJ_IX | SPIFFS_OP_C_READ,
            pFd->fileN, SPIFFS_PAGE_TO_PADDR(pfs, pageIXObjIX), SPIFFS_CFG_LOG_PAGE_SZ(pfs), pfs->pucWorkBuffer);
        SPIFFS_CHECK_RES(iRes);
        SPIFFS_VALIDATE_OBJIX(pObjIXHdr->p_hdr, pFd->objId, spanIXObjIXCur);
        pFd->cursor_objix_pix = pageIXObjIX;
        pFd->cursor_objix_spix = spanIXObjIXCur;
        pFd->uiOffset = uiCurSize;

        spanIXObjIXPrev = spanIXObjIXCur;
    }

    if (spanIXObjIXCur == 0) {
        // get data page from object index header page
        pageIXData = ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIXHdr + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)))[spanIXData];
        ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIXHdr + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)))[spanIXData] = SPIFFS_OBJ_ID_FREE;
    } else {
        // get data page from object index page
        pageIXData = ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIX + sizeof(SPIFFS_PAGE_OBJECT_IX)))[SPIFFS_OBJ_IX_ENTRY(pfs, spanIXData)];
        ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIX + sizeof(SPIFFS_PAGE_OBJECT_IX)))[SPIFFS_OBJ_IX_ENTRY(pfs, spanIXData)] = SPIFFS_OBJ_ID_FREE;
    }

    SPIFFS_DBG("truncate: got data pix "_SPIPRIpg"\n", pageIXData);

    if (uiNewSize == 0 || bIsRemoveFull || uiCurSize - uiNewSize >= SPIFFS_DATA_PAGE_SIZE(pfs)) {
        // delete full data page
        iRes = spiffs_page_data_check(pfs, pFd, pageIXData, spanIXData);
        if (iRes != SPIFFS_ERR_DELETED && iRes != SPIFFS_OK && iRes != SPIFFS_ERR_INDEX_REF_FREE) {
        SPIFFS_DBG("truncate: err validating data pix "_SPIPRIi"\n", iRes);
        break;
        }

        if (iRes == SPIFFS_OK) {
        iRes = spiffs_page_delete(pfs, pageIXData);
        if (iRes != SPIFFS_OK) {
            SPIFFS_DBG("truncate: err deleting data pix "_SPIPRIi"\n", iRes);
            break;
        }
        } else if (iRes == SPIFFS_ERR_DELETED || iRes == SPIFFS_ERR_INDEX_REF_FREE) {
        iRes = SPIFFS_OK;
        }

        // update current uiSize
        if (uiCurSize % SPIFFS_DATA_PAGE_SIZE(pfs) == 0) {
        uiCurSize -= SPIFFS_DATA_PAGE_SIZE(pfs);
        } else {
        uiCurSize -= uiCurSize % SPIFFS_DATA_PAGE_SIZE(pfs);
        }
        pFd->uiSize = uiCurSize;
        pFd->uiOffset = uiCurSize;
        SPIFFS_DBG("truncate: delete data page "_SPIPRIpg" for data spix:"_SPIPRIsp", uiCurSize:"_SPIPRIi"\n", pageIXData, spanIXData, uiCurSize);
    } else {
        // delete last page, partially
        SPIFFS_PAGE_HEADER p_hdr;
        SPIFFS_PAGE_IX new_data_pix;
        UINT32 bytes_to_remove = SPIFFS_DATA_PAGE_SIZE(pfs) - (uiNewSize % SPIFFS_DATA_PAGE_SIZE(pfs));
        SPIFFS_DBG("truncate: delete "_SPIPRIi" bytes from data page "_SPIPRIpg" for data spix:"_SPIPRIsp", uiCurSize:"_SPIPRIi"\n", bytes_to_remove, pageIXData, spanIXData, uiCurSize);

        iRes = spiffs_page_data_check(pfs, pFd, pageIXData, spanIXData);
        if (iRes != SPIFFS_OK) break;

        p_hdr.objId = pFd->objId & ~SPIFFS_OBJ_ID_IX_FLAG;
        p_hdr.span_ix = spanIXData;
        p_hdr.flags = 0xff;
        // allocate new page and copy unmodified data
        iRes = spiffs_page_allocate_data(pfs, pFd->objId & ~SPIFFS_OBJ_ID_IX_FLAG,
            &p_hdr, 0, 0, 0, 0, &new_data_pix);
        if (iRes != SPIFFS_OK) break;
        iRes = spiffs_phys_cpy(pfs, 0,
            SPIFFS_PAGE_TO_PADDR(pfs, new_data_pix) + sizeof(SPIFFS_PAGE_HEADER),
            SPIFFS_PAGE_TO_PADDR(pfs, pageIXData) + sizeof(SPIFFS_PAGE_HEADER),
            SPIFFS_DATA_PAGE_SIZE(pfs) - bytes_to_remove);
        if (iRes != SPIFFS_OK) break;
        // delete original data page
        iRes = spiffs_page_delete(pfs, pageIXData);
        if (iRes != SPIFFS_OK) break;
        p_hdr.flags &= ~SPIFFS_PH_FLAG_FINAL;
        iRes = _spiffs_wr(pfs, SPIFFS_OP_T_OBJ_DA | SPIFFS_OP_C_UPDT,
            pFd->fileN,
            SPIFFS_PAGE_TO_PADDR(pfs, new_data_pix) + offsetof(SPIFFS_PAGE_HEADER, flags),
            sizeof(UINT8),
            (UINT8 *)&p_hdr.flags);
        if (iRes != SPIFFS_OK) break;

        // update memory representation of object index page with new data page
        if (spanIXObjIXCur == 0) {
        // update object index header page
        ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIXHdr + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)))[spanIXData] = new_data_pix;
        SPIFFS_DBG("truncate: wrote page "_SPIPRIpg" to pObjIXHdr entry "_SPIPRIsp" in mem\n", new_data_pix, (SPIFFS_SPAN_IX)SPIFFS_OBJ_IX_ENTRY(pfs, spanIXData));
        } else {
        // update object index page
        ((SPIFFS_PAGE_IX*)((UINT8 *)pObjIX + sizeof(SPIFFS_PAGE_OBJECT_IX)))[SPIFFS_OBJ_IX_ENTRY(pfs, spanIXData)] = new_data_pix;
        SPIFFS_DBG("truncate: wrote page "_SPIPRIpg" to pObjIX entry "_SPIPRIsp" in mem\n", new_data_pix, (SPIFFS_SPAN_IX)SPIFFS_OBJ_IX_ENTRY(pfs, spanIXData));
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

        iRes = spiffs_page_index_check(pfs, pFd, pageIXObjIX, 0);
        SPIFFS_CHECK_RES(iRes);

        iRes = spiffs_page_delete(pfs, pageIXObjIX);
        SPIFFS_CHECK_RES(iRes);
        spiffs_cb_object_event(pfs, (SPIFFS_PAGE_OBJECT_IX *)0,
            SPIFFS_EV_IX_DEL, pFd->objId, 0, pageIXObjIX, 0);
        } else {
        // make uninitialized object
        SPIFFS_DBG("truncate: reset pObjIXHdr page "_SPIPRIpg"\n", pageIXObjIX);
        memset(pfs->pucWorkBuffer + sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER), 0xff,
            SPIFFS_CFG_LOG_PAGE_SZ(pfs) - sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER));
        iRes = spiffs_object_update_index_hdr(pfs, pFd, pFd->objId,
            pageIXObjIX, pfs->pucWorkBuffer, 0, 0, SPIFFS_UNDEFINED_LEN, &newPageIXObjIXHdr);
        SPIFFS_CHECK_RES(iRes);
        }
    } else {
        // update object index header page
        SPIFFS_DBG("truncate: update object index header page with indices and uiSize\n");
        iRes = spiffs_object_update_index_hdr(pfs, pFd, pFd->objId,
            pageIXObjIX, pfs->pucWorkBuffer, 0, 0, uiCurSize, &newPageIXObjIXHdr);
        SPIFFS_CHECK_RES(iRes);
    }
    } else {
    // update both current object index page and object index header page
    SPIFFS_PAGE_IX new_objix_pix;

    iRes = spiffs_page_index_check(pfs, pFd, pageIXObjIX, spanIXObjIXCur);
    SPIFFS_CHECK_RES(iRes);

    // move and update object index page
    iRes = spiffs_page_move(pfs, pFd->fileN, (UINT8*)pObjIXHdr, pFd->objId, 0, pageIXObjIX, &new_objix_pix);
    SPIFFS_CHECK_RES(iRes);
    spiffs_cb_object_event(pfs, (SPIFFS_PAGE_OBJECT_IX *)pObjIXHdr,
        SPIFFS_EV_IX_UPD, pFd->objId, pObjIX->p_hdr.span_ix, new_objix_pix, 0);
    SPIFFS_DBG("truncate: store modified pObjIX page, "_SPIPRIpg":"_SPIPRIsp"\n", new_objix_pix, spanIXObjIXCur);
    pFd->cursor_objix_pix = new_objix_pix;
    pFd->cursor_objix_spix = spanIXObjIXCur;
    pFd->uiOffset = uiCurSize;
    // update object index header page with new uiSize
    iRes = spiffs_object_update_index_hdr(pfs, pFd, pFd->objId,
        pFd->pageIXObjIXHdr, 0, 0, 0, uiCurSize, &newPageIXObjIXHdr);
    SPIFFS_CHECK_RES(iRes);
    }
    pFd->uiSize = uiCurSize;

    return iRes;
}