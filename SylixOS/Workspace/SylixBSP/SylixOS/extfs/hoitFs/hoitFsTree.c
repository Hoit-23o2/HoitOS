/*********************************************************************************************************
**
**                                    ÖÐ¹úÈí¼þ¿ªÔ´×éÖ¯
**
**                                   Ç¶ÈëÊ½ÊµÊ±²Ù×÷ÏµÍ³
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------ÎÄ¼þÐÅÏ¢--------------------------------------------------------------------------------
**
** ÎÄ   ¼þ   Ãû: hoitFsTree.c
**
** ´´   ½¨   ÈË: Pan yanqi (ÅËÑÓ÷è)
**
** ÎÄ¼þ´´½¨ÈÕÆÚ: 2021 Äê 03 ÔÂ 28 ÈÕ
**
** Ãè        Êö: JFFS2 Like Frag TreeÊµÏÖ
*********************************************************************************************************/

#include "hoitFsTree.h"
#include "hoitFsFDLib.h"
#include "hoitFsCache.h"

#ifdef FT_TEST
PHOIT_FULL_DNODE __hoit_truncate_full_dnode(PHOIT_VOLUME pfs, PHOIT_FULL_DNODE pFDnode, UINT uiOffset, UINT uiSize){
    PHOIT_FULL_DNODE pFDNode = (PHOIT_FULL_DNODE)lib_malloc(sizeof(HOIT_FULL_DNODE));
    pFDNode->HOITFD_length = uiSize;
    pFDNode->HOITFD_offset = uiOffset;
    return pFDNode;
}

BOOL __hoit_delete_full_dnode(PHOIT_VOLUME pfs, PHOIT_FULL_DNODE pFDnode, BOOL bDoDelete){
    if(!bDoDelete){
        lib_free(pFDnode);
    }
    return LW_TRUE;
}


#endif //FT_TEST

#define FT_GET_KEY(pFTn)            pFTn->pRbn.iKey
#define FT_LEFT_CHILD(pFTn)         (PHOIT_FRAG_TREE_NODE)pFTnRoot->pRbn.pRbnLeft
#define FT_RIGHT_CHILD(pFTn)        (PHOIT_FRAG_TREE_NODE)pFTnRoot->pRbn.pRbnRight
#define RB_GUARD(pFTTree)           pFTTree->pRbTree->pRbnGuard

#define MAX(a, b)                   ((a) > (b) ? (a) : (b))
#define MIN(a, b)                   ((a) < (b) ? (a) : (b))    


PHOIT_FRAG_TREE_NODE __hoitFragTreeGetMinimum(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTnRoot){
    PHOIT_RB_NODE       pRbnTraverse; 
    pRbnTraverse    =   &pFTnRoot->pRbn;
    while (pRbnTraverse->pRbnLeft != RB_GUARD(pFTTree))
    {
        pRbnTraverse = pRbnTraverse->pRbnLeft;
    }
    return (PHOIT_FRAG_TREE_NODE)pRbnTraverse;
}

PHOIT_FRAG_TREE_NODE __hoitFragTreeGetSuccessor(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTnRoot){
    PHOIT_RB_NODE pRbnTraverse;
    PHOIT_RB_NODE pRbn;

    pRbn = &pFTnRoot->pRbn;
    if(pRbn->pRbnRight != RB_GUARD(pFTTree)){
        return __hoitFragTreeGetMinimum(pFTTree, FT_RIGHT_CHILD(pFTnRoot));
    }

    pRbnTraverse = pRbn->pRbnParent;
    while (pRbnTraverse != RB_GUARD(pFTTree) && pRbn == pRbnTraverse->pRbnRight)
    {
        pRbn = pRbnTraverse;
        pRbnTraverse = pRbnTraverse->pRbnParent;
    }
    return (PHOIT_FRAG_TREE_NODE)pRbnTraverse;
}

/*********************************************************************************************************
** ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: __hoitFragTreeCollectRangeHelper
** ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Õ¼ï¿½iKeyLowï¿½ï¿½iKeyHighÖ®ï¿½ï¿½Ä½Úµï¿?
** ï¿½ä¡¡ï¿½ï¿½  : pFTlistHeader          ï¿½ï¿½ï¿½ï¿½Í·
**            pFTTree               FragTree
**            pFTnRoot              ï¿½ï¿½ï¿½Úµï¿½
**            iKeyLow               ï¿½Í¼ï¿½Öµ
**            iKeyHigh              ï¿½ß¼ï¿½Öµ
** ï¿½ä¡¡ï¿½ï¿½  : None
** È«ï¿½Ö±ï¿½ï¿½ï¿½:
** ï¿½ï¿½ï¿½ï¿½Ä£ï¿½ï¿½:
*********************************************************************************************************/
VOID __hoitFragTreeCollectRangeHelper(PHOIT_FRAG_TREE_LIST_HEADER pFTlistHeader,
                                      PHOIT_FRAG_TREE pFTTree, 
                                      PHOIT_FRAG_TREE_NODE pFTnRoot, 
                                      INT32 iKeyLow, 
                                      INT32 iKeyHigh){
    PHOIT_FRAG_TREE_LIST_NODE   pFTlistNode;
    PHOIT_FRAG_TREE_NODE        pFTSuccessor;
    BOOL                        bHasSuccessor = LW_TRUE;
    BOOL                        bIsInRange = LW_FALSE;


    if(&pFTnRoot->pRbn == RB_GUARD(pFTTree)){
        return;
    }
    __hoitFragTreeCollectRangeHelper(pFTlistHeader, 
                                     pFTTree, 
                                     FT_LEFT_CHILD(pFTnRoot), 
                                     iKeyLow,
                                     iKeyHigh);
    pFTSuccessor =  __hoitFragTreeGetSuccessor(pFTTree, pFTnRoot);
    if(&pFTSuccessor->pRbn == RB_GUARD(pFTTree)){
        bHasSuccessor = LW_FALSE;
    }
    /* 
        Case 1:         [key1    iKeyLow    key2 ...]             ï¿½Â½ç£ºkey1
        Case 2:         [key1    iKeyLow]                         ï¿½Â½ç£ºkey1
    */
    if((bHasSuccessor && (FT_GET_KEY(pFTnRoot) <= iKeyLow && FT_GET_KEY(pFTSuccessor) > iKeyLow)) ||
       (!bHasSuccessor && FT_GET_KEY(pFTnRoot) <= iKeyLow)){                                          /* iKey < iKeyLow ï¿½ï¿½ iKeyï¿½Äºï¿½Ì½Úµï¿½ï¿½iKey > iKeyLow*/
        bIsInRange = LW_TRUE;
        pFTlistHeader->uiLowBound = FT_GET_KEY(pFTnRoot);
    }
    /* 
        Case 3:         [iKeyLow   key1   key2 ...]               ï¿½Â½ç£ºkey1
        --------------------------------------------------------------------
        Case 1:         [iKeyLow   key1   iKeyHigh]               ï¿½Ï½ç£ºkey1
        Case 2:         [iKeyLow   key1   iKeyHigh    key2]       ï¿½Ï½ç£ºkey2
        Case 3:         [ikey      iKeyLow   iKeyHigh]            ï¿½Ï½ç£ºkey1
    */
    else if (FT_GET_KEY(pFTnRoot) > iKeyLow && FT_GET_KEY(pFTnRoot) <= iKeyHigh){                     /* iKey > iKeyLow ï¿½ï¿½ iKey < iKeyHigh */
        bIsInRange = LW_TRUE;
        if(pFTlistHeader->uiLowBound == INT_MIN){
            pFTlistHeader->uiLowBound = FT_GET_KEY(pFTnRoot);
        }
        if(bHasSuccessor){
            if(FT_GET_KEY(pFTSuccessor) > iKeyHigh){
                pFTlistHeader->uiHighBound =  FT_GET_KEY(pFTnRoot);
            }
        }
        else{
            pFTlistHeader->uiHighBound = FT_GET_KEY(pFTnRoot);
        }
    }

    if(bIsInRange){
        pFTlistNode = newFragTreeListNode(pFTnRoot);
        hoitFragTreeListInsertNode(pFTlistHeader, pFTlistNode);
        pFTlistHeader->uiNCnt++;
#ifdef FT_DEBUG
        printf("Collecting Node %d \n", pFTlistNode->pFTn->pRbn.iKey);
#endif // FT_DEBUG
    }
    if (FT_GET_KEY(pFTnRoot) > iKeyHigh)                                                /* ï¿½ï¿½Ö¦ */
    {
        return;
    }
    __hoitFragTreeCollectRangeHelper(pFTlistHeader, 
                                     pFTTree, 
                                     FT_RIGHT_CHILD(pFTnRoot), 
                                     iKeyLow,
                                     iKeyHigh);
}   
/*********************************************************************************************************
** ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: __hoitFragTreeConquerNode
** ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: ï¿½ï¿½ï¿½ï¿½Ò»ï¿½ï¿½ï¿½Úµã£¬ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Þ¸Ä£ï¿?4ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ä£Ê½
** ï¿½ä¡¡ï¿½ï¿½  : pFTTree                 FragTree
**           pFTn                   ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Úµï¿½
**           uiConquerorLow          ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Â½ï¿½
**           uiConquerorHigh         ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ï½ï¿½
**           pFTnNew                 ï¿½ï¿½ï¿½Ú·ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Úµï¿½
**           uiCase                  ï¿½ï¿½ï¿½Ú·ï¿½ï¿½ï¿½Overlayï¿½ï¿½ï¿?
**           bDoDelete               ï¿½ï¿½ï¿?2ï¿½ï¿½3ï¿½ï¿½ï¿½æ¼°ï¿½ï¿½É¾ï¿½ï¿½ï¿½ï¿½ï¿½Ç·ï¿½É¾ï¿½ï¿½RawInfoï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½RawNodeÎªï¿½ï¿½ï¿½Ú½Úµï¿½
** ï¿½ä¡¡ï¿½ï¿½  : ï¿½ï¿½ï¿½ï¿½ï¿½É¹ï¿½ï¿½ï¿½ï¿½ï¿½LW_TRUEï¿½ï¿½ï¿½ï¿½ï¿½ò·µ»ï¿½LW_FALSE
** È«ï¿½Ö±ï¿½ï¿½ï¿½:
** ï¿½ï¿½ï¿½ï¿½Ä£ï¿½ï¿½:
*********************************************************************************************************/
BOOL __hoitFragTreeConquerNode(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn, 
                               UINT32 uiConquerorLow, UINT32 uiConquerorHigh, PHOIT_FRAG_TREE_NODE *pFTnNew, 
                               UINT8 * uiCase, BOOL bDoDelete) {
    PHOIT_FULL_DNODE          pFDNodeNew;
    PHOIT_FRAG_TREE_LIST_NODE pFTlistNodeNew;
    

    UINT32                    uiLeftRemainSize;
    UINT32                    uiRightRemainSize;

    UINT32                    uiCurLow;
    UINT32                    uiCurHigh;

    BOOL                      bIsOverlay;

    uiCurLow                  = pFTn->uiOfs;
    uiCurHigh                 = uiCurLow + pFTn->uiSize;
    bIsOverlay                = MAX(uiCurLow, uiConquerorLow) <= MIN(uiCurHigh, uiConquerorHigh);
    *uiCase                   = -1;
    *pFTnNew                  = LW_NULL;



    if(bIsOverlay){                                                                     /* ï¿½ï¿½Ê¼ï¿½ï¿½ï¿½ï¿½ */
        /* 
            |-------|-------------|-------|
            ^       ^             ^       ^
            Cur  Conqueror       Cur    Conqueror  
        */
        if(uiCurLow < uiConquerorLow && uiCurHigh <= uiConquerorHigh){                  /* ï¿½ï¿½ï¿?1 */
            uiLeftRemainSize = uiConquerorLow - uiCurLow;
            pFTn->uiSize = uiLeftRemainSize;
            pFTn->pFDnode->HOITFD_length = uiLeftRemainSize;
            *uiCase = 1;
        }
        /* 
            |-------|-------------|-------|
            ^       ^             ^       ^
            Cur  Conqueror      Conqueror  Cur
        */
        else if (uiCurLow < uiConquerorLow && uiCurHigh > uiConquerorHigh)              /* ï¿½ï¿½ï¿?2 */
        {
            uiLeftRemainSize = uiCurLow - uiConquerorLow;
            uiRightRemainSize = uiConquerorHigh - uiCurHigh;

            pFDNodeNew = __hoit_truncate_full_dnode(pFTTree->pfs,                       /* ï¿½Ø¶ï¿½ï¿½Ò±ß²ï¿½ï¿½Ö£ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ÂµÄ½Úµï¿½ */
                                                    pFTn->pFDnode,
                                                    uiConquerorHigh - uiCurLow,
                                                    uiRightRemainSize);

            pFTn->uiSize = uiLeftRemainSize;                                            /* ï¿½Þ¸ï¿½ï¿½ï¿½ß²ï¿½ï¿½Öµï¿½size */
            pFTn->pFDnode->HOITFD_length = uiLeftRemainSize;

            *pFTnNew = newHoitFragTreeNode(pFDNodeNew, pFDNodeNew->HOITFD_length,       /* ï¿½ï¿½ï¿½ï¿½pFDNodeNewï¿½Â½ï¿½FragTreeNode */
                                          pFDNodeNew->HOITFD_offset, pFDNodeNew->HOITFD_offset);

            hoitFragTreeInsertNode(pFTTree, *pFTnNew);                                   /* ï¿½ï¿½ï¿½ï¿½Úµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ */
            *uiCase = 2;
        }
        /* 
                |-------|-------------|-------|
                ^       ^             ^       ^
            Conqueror  Cur       Conqueror  Cur
        */
        else if (uiCurLow >= uiConquerorLow && uiCurHigh > uiConquerorHigh)             /* ï¿½ï¿½ï¿?3 */
        {
            uiRightRemainSize = uiCurHigh - uiConquerorHigh;
            pFDNodeNew = __hoit_truncate_full_dnode(pFTTree->pfs,
                                                    pFTn->pFDnode,
                                                    uiConquerorHigh - uiCurLow,
                                                    uiRightRemainSize);
            *pFTnNew = newHoitFragTreeNode(pFDNodeNew, pFDNodeNew->HOITFD_length,       /* ï¿½ï¿½ï¿½ï¿½pFDNodeNewï¿½Â½ï¿½FragTreeNode */
                                           pFDNodeNew->HOITFD_offset, pFDNodeNew->HOITFD_offset);
            hoitFragTreeInsertNode(pFTTree, *pFTnNew);
            hoitFragTreeDeleteNode(pFTTree, pFTn, bDoDelete);                            /* É¾ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Úµï¿? */
            *uiCase = 3;
        }
        /* 
            |-------|-------------|-------|
            ^       ^             ^       ^
        Conqueror  Cur           Cur    Conqueror
        */
        else if (uiCurLow >= uiConquerorLow && uiCurHigh <= uiConquerorHigh)            /* ï¿½ï¿½ï¿?4 */
        {
            hoitFragTreeDeleteNode(pFTTree, pFTn, bDoDelete);                            /* É¾ï¿½ï¿½ï¿½ï¿½Ç°FragTreeï¿½ÏµÄ½Úµï¿½ */
            *uiCase = 4;
        }
    }
    return bIsOverlay;
}
/*********************************************************************************************************
** º¯ÊýÃû³Æ: hoitInitFragTree
** ¹¦ÄÜÃèÊö: ³õÊ¼»¯FragTree
** Êä¡¡Èë  : None
** Êä¡¡³ö  : ·µ»ØFragTree
** È«¾Ö±äÁ¿:
** µ÷ÓÃÄ£¿é:
*********************************************************************************************************/
PHOIT_FRAG_TREE hoitInitFragTree(PHOIT_VOLUME pfs){
    PHOIT_FRAG_TREE         pFTTree;

    pFTTree = (PHOIT_FRAG_TREE)lib_malloc(sizeof(HOIT_FRAG_TREE));
    pFTTree->pRbTree = hoitRbInitTree();
    pFTTree->uiNCnt = 0;
    pFTTree->pfs = pfs;
    return pFTTree;
}
/*********************************************************************************************************
** º¯ÊýÃû³Æ: hoitFragTreeInsertNode
** ¹¦ÄÜÃèÊö: ÏòFragTreeÖÐ²åÈë½Úµã
** Êä¡¡Èë  : pFTTree            FragTree
**           pFTn               ´ý²åÈëµÄ½Úµã   
** Êä¡¡³ö  : ·µ»Ø²åÈëµÄ½Úµã
** È«¾Ö±äÁ¿:
** µ÷ÓÃÄ£¿é:
*********************************************************************************************************/
PHOIT_FRAG_TREE_NODE hoitFragTreeInsertNode(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn){
    hoitRbInsertNode(pFTTree->pRbTree, &pFTn->pRbn);
    pFTTree->uiNCnt++;
    return pFTn;
}
/*********************************************************************************************************
** º¯ÊýÃû³Æ: hoitFragTreeSearchNode
** ¹¦ÄÜÃèÊö: ÔÚFragTreeÉÏËÑË÷¼üÖµÎªiKeyµÄ½Úµã
** Êä¡¡Èë  : pFTTree          FragTree
**           iKey             ¼üÖµ
** Êä¡¡³ö  : ÕÒµ½¾Í·µ»ØFragTree½Úµã£¬·ñÔò·µ»ØLW_NULL
** È«¾Ö±äÁ¿:
** µ÷ÓÃÄ£¿é:
*********************************************************************************************************/
PHOIT_FRAG_TREE_NODE hoitFragTreeSearchNode(PHOIT_FRAG_TREE pFTTree, INT32 iKey){
    PHOIT_RB_NODE           pRbn; 
    PHOIT_FRAG_TREE_NODE    pFTn;

    pRbn = hoitRbSearchNode(pFTTree->pRbTree, iKey);
    pFTn = (PHOIT_FRAG_TREE_NODE)(pRbn);  
    return pFTn;
}
/*********************************************************************************************************
** ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: hoitFragTreeSearchNode
** ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: ï¿½ï¿½FragTreeï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Öµï¿½Ú·ï¿½Î§[x, y]ï¿½Ä½Úµã£¬ï¿½ï¿½ï¿½Ð£ï¿½x <= iKeyLowï¿½ï¿½y >= iKeyHighï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
** ï¿½ä¡¡ï¿½ï¿½  : pFTTree          FragTree
**           iKeyLow          ï¿½Í¼ï¿½Öµ
**           iKeyHigh         ï¿½ß¼ï¿½Öµ
** ï¿½ä¡¡ï¿½ï¿½  : ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Í·
** È«ï¿½Ö±ï¿½ï¿½ï¿½:
** ï¿½ï¿½ï¿½ï¿½Ä£ï¿½ï¿½:
*********************************************************************************************************/
PHOIT_FRAG_TREE_LIST_HEADER hoitFragTreeCollectRange(PHOIT_FRAG_TREE pFTTree, INT32 iKeyLow, INT32 iKeyHigh){
    PHOIT_FRAG_TREE_LIST_HEADER     pFTlistHeader;
    pFTlistHeader = hoitFragTreeListInit();
    __hoitFragTreeCollectRangeHelper(pFTlistHeader, 
                                     pFTTree, 
                                     (PHOIT_FRAG_TREE_NODE)pFTTree->pRbTree->pRbnRoot,
                                     iKeyLow,
                                     iKeyHigh);
    if(pFTlistHeader->uiHighBound == INT_MAX){
        pFTlistHeader->uiHighBound = pFTlistHeader->uiLowBound;
    }
#ifdef FT_DEBUG
    printf("Collecting Over\n");
#endif //FT_DEBUG
    return pFTlistHeader;
}
/*********************************************************************************************************
** º¯ÊýÃû³Æ: hoitFragTreeDeleteNode
** ¹¦ÄÜÃèÊö: ÔÚFragTreeÉÏÉ¾³ýÄ³¸ö½Úµã£¬²¢ÊÍ·ÅÆäÄÚ´æ£¬×¢Òâ±£´æ
** Êä¡¡Èë  : pFTTree          FragTree
**           pFTn             FragTreeÉÏµÄÄ³¸ö½Úµã
** Êä¡¡³ö  : É¾³ý³É¹¦·µ»ØTrue£¬·ñÔò·µ»ØFalse
** È«¾Ö±äÁ¿:
** µ÷ÓÃÄ£¿é:
*********************************************************************************************************/
BOOL hoitFragTreeDeleteNode(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn, BOOL bDoDelete){
    BOOL        res; 
    if(pFTTree->uiNCnt == 0)
        return LW_FALSE;
    
    res = hoitRbDeleteNode(pFTTree->pRbTree, &pFTn->pRbn);
    if(res){
        pFTTree->uiNCnt--;
        __hoit_delete_full_dnode(pFTTree->pfs, pFTn->pFDnode, bDoDelete); /* É¾ï¿½ï¿½DnodeÖ¸ï¿½ï¿½ï¿½ï¿½Ú´ï¿½Õ¼ï¿½ */
        lib_free(pFTn);                                                   /* É¾ï¿½ï¿½pFTnÖ¸ï¿½ï¿½ï¿½ï¿½Ú´ï¿½Õ¼ï¿½ */
    }
    return res;
}



/*********************************************************************************************************
** ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: hoitFragTreeDeleteNode
** ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: ï¿½ï¿½FragTreeï¿½ï¿½É¾ï¿½ï¿½Ä³ï¿½ï¿½Î§ï¿½ÚµÄ½Úµï¿½
** ï¿½ä¡¡ï¿½ï¿½  : pFTTree          FragTree
**           iKeyLow          ï¿½Í¼ï¿½Öµ
**           iKeyHigh         ï¿½ß¼ï¿½Öµ
**           bDoDelete        ï¿½Ç·ï¿½É¾ï¿½ï¿½RawInfoï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½RawNodeÎªï¿½ï¿½ï¿½Ú½Úµï¿½
** ï¿½ä¡¡ï¿½ï¿½  : É¾ï¿½ï¿½ï¿½É¹ï¿½ï¿½ï¿½ï¿½ï¿½Trueï¿½ï¿½ï¿½ï¿½ï¿½ò·µ»ï¿½False
** È«ï¿½Ö±ï¿½ï¿½ï¿½:
** ï¿½ï¿½ï¿½ï¿½Ä£ï¿½ï¿½:
*********************************************************************************************************/
BOOL hoitFragTreeDeleteRange(PHOIT_FRAG_TREE pFTTree, INT32 iKeyLow, INT32 iKeyHigh, BOOL bDoDelete){
    PHOIT_FRAG_TREE_LIST_HEADER pFTlistHeader;
    PHOIT_FRAG_TREE_LIST_NODE   pFTlistNode;
    
    PHOIT_FRAG_TREE_NODE        pFTnNew;

    BOOL                        bRes = LW_TRUE;
    UINT                        uiCount = 0;
    UINT8                       uiCase;

    UINT32                      uiConquerorLow = iKeyLow;
    UINT32                      uiConquerorHigh = iKeyHigh;

    pFTlistHeader = hoitFragTreeCollectRange(pFTTree, iKeyLow, iKeyHigh);
    pFTlistNode = pFTlistHeader->pFTlistHeader->pFTlistNext;
    while (pFTlistNode)
    {
        uiCount++;
#ifdef FT_DEBUG
        printf("count %d\n", uiCount);
#endif
        
        //TODO: ï¿½ï¿½Ö¤ï¿½ß¼ï¿½
        __hoitFragTreeConquerNode(pFTTree, pFTlistNode->pFTn, uiConquerorLow, uiConquerorHigh, 
                                  &pFTnNew, &uiCase, bDoDelete);
        
        pFTlistNode = pFTlistNode->pFTlistNext;
    }
    hoitFragTreeListFree(pFTlistHeader);
    return bRes;    
}

/*********************************************************************************************************
** ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: hoitFragTreeDeleteTree
** ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: É¾ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½FragTreeï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Í·ï¿½pFTTreeï¿½ï¿½ï¿½Ú´ï¿½
** ï¿½ä¡¡ï¿½ï¿½  : pFTTree          FragTree
** ï¿½ä¡¡ï¿½ï¿½  : É¾ï¿½ï¿½ï¿½É¹ï¿½ï¿½ï¿½ï¿½ï¿½Trueï¿½ï¿½ï¿½ï¿½ï¿½ò·µ»ï¿½False
** È«ï¿½Ö±ï¿½ï¿½ï¿½:
** ï¿½ï¿½ï¿½ï¿½Ä£ï¿½ï¿½:
*********************************************************************************************************/
BOOL hoitFragTreeDeleteTree(PHOIT_FRAG_TREE pFTTree, BOOL bDoDelete){
    BOOL                          res;
    res = hoitFragTreeDeleteRange(pFTTree, INT_MIN, INT_MAX, bDoDelete);
    lib_free(pFTTree->pRbTree->pRbnGuard);
    lib_free(pFTTree->pRbTree);
    lib_free(pFTTree);
    return res;
}

/*********************************************************************************************************
** º¯ÊýÃû³Æ: hoitFragTreeTraverse
** ¹¦ÄÜÃèÊö: ÖÐÐò±éÀúFragTree
** Êä¡¡Èë  : pFTTree          FragTree
**           pFTnRoot         FragTree×ÓÊ÷¸ù
** Êä¡¡³ö  : None
** È«¾Ö±äÁ¿:
** µ÷ÓÃÄ£¿é:
*********************************************************************************************************/
VOID hoitFragTreeTraverse(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTnRoot){
    if(&pFTnRoot->pRbn == RB_GUARD(pFTTree)){
        return;
    }
    else
    {
        hoitFragTreeTraverse(pFTTree, FT_LEFT_CHILD(pFTnRoot));
        printf("uiOfs: %d, uiSize: %d, iKey: %d \n", pFTnRoot->uiOfs, pFTnRoot->uiSize, FT_GET_KEY(pFTnRoot));
        hoitFragTreeTraverse(pFTTree, FT_RIGHT_CHILD(pFTnRoot));
    }
}
/*********************************************************************************************************
** ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: hoitFragTreeRead
** ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½FragTree
** ï¿½ä¡¡ï¿½ï¿½  : pFTTree          FragTree
**           uiOfs            ï¿½ï¿½ï¿½ï¿½Ä¼ï¿½ï¿½ï¿½Æ?¿½ï¿?
**           uiSize           ï¿½ï¿½ï¿½ï¿½
**           pContent         ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Î»ï¿½ï¿½
** ï¿½ä¡¡ï¿½ï¿½  : None
** È«ï¿½Ö±ï¿½ï¿½ï¿½:
** ï¿½ï¿½ï¿½ï¿½Ä£ï¿½ï¿½:
*********************************************************************************************************/
error_t hoitFragTreeRead(PHOIT_FRAG_TREE pFTTree, UINT32 uiOfs, UINT32 uiSize, PCHAR pContent){
    UINT32                      iKeyLow;
    UINT32                      iKeyHigh;
    UINT32                      uiBias;
    PHOIT_FRAG_TREE_LIST_NODE   pFTlist;
    PHOIT_FRAG_TREE_LIST_HEADER pFTlistHeader;

    UINT32                      uiPhyOfs;
    UINT32                      uiPhySize;

    UINT32                      uiSizeRead;
    UINT32                      uiSizeRemain;

    UINT32                      uiPerOfs;
    UINT32                      uiPerSize;
    PCHAR                       pPerContent;

    uiSizeRead          = 0;
    uiSizeRemain        = uiSize;
    iKeyLow             = uiOfs;
    iKeyHigh            = uiOfs + uiSize;

    pFTlistHeader       = hoitFragTreeCollectRange(pFTTree, iKeyLow, iKeyHigh);
    uiBias              = uiOfs - pFTlistHeader->uiLowBound; 

    pFTlist             = pFTlistHeader->pFTlistHeader->pFTlistNext;
    while (pFTlist != LW_NULL)
    {
        uiPhyOfs = pFTlist->pFTn->pFDnode->HOITFD_raw_info->phys_addr + sizeof(HOIT_RAW_HEADER);
        uiPhySize = pFTlist->pFTn->pFDnode->HOITFD_length;
        /* ï¿½ï¿½ï¿½×¸ï¿½ï¿½ï¿½ï¿½ï¿½Êµï¿½ï¿½
            |--H-|-uiBias-|
            |----|--------|-------|
                 |        |       |
                 |        |       |
             uiPhyOfs  uiPerOfs  uiPhyOfs + uiPhySize
        */
        if(uiSizeRead == 0){
            uiPerOfs = uiPhyOfs + uiBias;
            uiPerSize = uiPhySize - uiBias;
        }
        /* ï¿½ï¿½ï¿½ï¿½ï¿½Ò»ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Êµï¿½ï¿?
                        |--H-|-remain-|  
                        |----|--------|-------|
                             |                |
                             |                |
                uiPhyOfs(uiPerOfs)  uiPhyOfs + uiPhySize
        */
        else if (uiSizeRead + uiPhySize >= uiSize)
        {
            uiPerOfs = uiPhySize;
            uiPerSize = uiSizeRemain;
        }
        else                                                                        /* ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿? */
        {
            uiPerOfs = uiPhySize;
            uiPerSize = uiPhySize;
        }

        pPerContent = (PCHAR)lib_malloc(uiPerSize);                                 /* ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ */
        //TODO: ï¿½Ó»ï¿½ï¿½ï¿½ï¿½Ï¶ï¿½
        hoitReadFromCache(uiPerOfs, pPerContent, uiPerSize);

        lib_memcpy(pContent + uiSizeRead, pPerContent, uiPerSize);
        lib_free(pPerContent);
        
        pFTlist = pFTlist->pFTlistNext;
        uiSizeRead += uiPerSize;
        uiSizeRemain -= uiPerSize;
    }

    hoitFragTreeListFree(pFTlistHeader);                                             /* ï¿½Í·ï¿½ï¿½ï¿½ï¿½ï¿½ */
    return ERROR_NONE;
}

/*********************************************************************************************************
** ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: hoitFragTreeOverlayFixUp
** ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½: ï¿½Þ¸ï¿½FragTreeï¿½ï¿½ï¿½Øµï¿½ï¿½Ä½Úµï¿½
** ï¿½ä¡¡ï¿½ï¿½  : pFTTree          FragTree
** ï¿½ä¡¡ï¿½ï¿½  : ï¿½ï¿½ï¿½Þ¸ï¿½ï¿½ï¿½ï¿½LW_TRUEï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½LW_FALSE
** È«ï¿½Ö±ï¿½ï¿½ï¿½:
** ï¿½ï¿½ï¿½ï¿½Ä£ï¿½ï¿½:
*********************************************************************************************************/
BOOL hoitFragTreeOverlayFixUp(PHOIT_FRAG_TREE pFTTree){
    PHOIT_FRAG_TREE_LIST_HEADER     pFTlistHeader;
    PHOIT_FRAG_TREE_LIST_NODE       pFTlistCur;
    PHOIT_FRAG_TREE_LIST_NODE       pFTlistConqueror;
    PHOIT_FRAG_TREE_LIST_NODE       pFTlistNext;

    PHOIT_FRAG_TREE_NODE            pFTnConqueror;

    PHOIT_FRAG_TREE_NODE            pFTnNew;
    PHOIT_FRAG_TREE_NODE            pFTn;
    PHOIT_FRAG_TREE_LIST_NODE       pFTlistNodeNew;
    
    UINT32                          uiCurLow;                                 /* ï¿½ï¿½ï¿½ä£º[curLow, curHigh) */
    UINT32                          uiCurHigh;

    UINT32                          uiConquerorLow;
    UINT32                          uiConquerorHigh;
    UINT8                           uiCase;

    UINT32                          uiLeftRemainSize;
    UINT32                          uiRightRemainSize;
    UINT32                          uiRightOffset;

    BOOL                            bIsOverlay;

    pFTlistHeader = hoitFragTreeCollectRange(pFTTree, INT_MIN, INT_MAX);
    pFTlistCur = pFTlistHeader->pFTlistHeader->pFTlistNext;
    
    while (pFTlistCur != LW_NULL)                     /* ï¿½ï¿½ï¿½Î?¿½ï¿½É?¿½ï¿½ï¿½Ú²ï¿½Î?¿½ï¿½É? */
    {
        uiCurLow = pFTlistCur->pFTn->uiOfs;
        uiCurHigh = uiCurLow + pFTlistCur->pFTn->uiSize;
        //TODO: ï¿½ï¿½ï¿½ï¿½ï¿½ï¿ªÊ¼ï¿½ï¿½ï¿½ï¿½Øµï¿½ï¿½ï¿½ï¿½ï¿½Ò?¿½ï¿½ï¿½ï¿½ï¿½ï¿?
        pFTlistConqueror = pFTlistHeader->pFTlistHeader->pFTlistNext;               /* Ö¸ï¿½ï¿½ï¿½Ò»ï¿½ï¿½ï¿½ï¿½Ð§ï¿½Úµï¿? */
        while (pFTlistConqueror != LW_NULL)
        {
            bIsOverlay = LW_FALSE;
            if(pFTlistConqueror == pFTlistCur)                                    /* ï¿½Ô¼ï¿½ï¿½Í²ï¿½ï¿½ï¿½ï¿½Ô¼ï¿½ï¿½È½ï¿½ï¿½ï¿½ */
            {
                continue;
            }
            if(pFTlistConqueror->pFTn->pFDnode->HOITFD_version                      /* listInnerï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ß£ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½versionï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ú±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ß²ï¿½ï¿½Ü½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿? */
             < pFTlistCur->pFTn->pFDnode->HOITFD_version){
                continue;
            }

            pFTn            = pFTlistCur->pFTn;
            pFTnConqueror   = pFTlistConqueror->pFTn;
            uiConquerorLow  = pFTnConqueror->uiOfs;
            uiConquerorHigh = uiConquerorLow + pFTnConqueror->uiSize;
            
            bIsOverlay = __hoitFragTreeConquerNode(pFTTree, pFTn, uiConquerorLow, uiConquerorHigh,
                                                   &pFTnNew, &uiCase, LW_TRUE);
            if(bIsOverlay){
                switch (uiCase)
                {
                /*! ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ì£ï¿½
                    1. ï¿½ï¿½ï¿½ï¿½Curï¿½ï¿½Ó¦ï¿½Úµï¿½ï¿½Sizeï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½

                    |-------|-------------|-------|
                    ^       ^             ^       ^
                    Cur  Conqueror       Cur    Conqueror  
                */
                case 1:
                    pFTlistConqueror = pFTlistConqueror->pFTlistNext;
                    break;
                /*! ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ì£ï¿½
                    1. ï¿½ï¿½ï¿½ï¿½Curï¿½Úµã£¬ï¿½ï¿½ï¿½ï¿½ï¿½Âµï¿½FragTreeï¿½Úµï¿½pFTnNewï¿½ï¿½ï¿½ä³¤ï¿½ï¿½ÎªCurHigh - ConquerorHighï¿½ï¿½
                       Æ«ï¿½ï¿½ÎªConquerorHigh - CurLowï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½FragTreeï¿½ï¿½
                    2. ï¿½ï¿½ï¿½ï¿½Curï¿½ï¿½Ó¦ï¿½Úµï¿½ï¿½SizeÎªConquereorLow - CurLow
                    3. ï¿½ï¿½ï¿½Â½Úµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ä©Î²ï¿½Ð£ï¿½ï¿½ï¿½ï¿½ï¿½Ò»ï¿½ï¿½ï¿½Ä¼ï¿½ï¿½
                    |-------|-------------|-------|
                    ^       ^             ^       ^
                    Cur  Conqueror      Conqueror  Cur
                */
                case 2:
                    if(pFTnNew == LW_NULL){
                        printf("something wrong with [fragtree fixup] CASE 2 when fix overlay of nodes containing %d and %d \n", 
                                FT_GET_KEY(pFTn), FT_GET_KEY(pFTnConqueror));
                        return;
                    }
                    else {
                        pFTlistNodeNew = newFragTreeListNode(pFTnNew);
                        hoitFragTreeListInsertNode(pFTlistHeader, pFTlistNodeNew);                  /* ï¿½ï¿½ï¿½ï¿½Úµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿? */

                        pFTlistConqueror = pFTlistConqueror->pFTlistNext;
                    }
                    break;
                /*! ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ì£ï¿½
                    1. ï¿½ï¿½ï¿½ï¿½Curï¿½Úµã£¬ï¿½ï¿½ï¿½ï¿½ï¿½Âµï¿½FragTreeï¿½Úµï¿½pFTnNewï¿½ï¿½ï¿½ä³¤ï¿½ï¿½ÎªCurHigh - ConquerorHighï¿½ï¿½
                       Æ«ï¿½ï¿½ÎªConquerorHigh - CurLowï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½FragTreeï¿½ï¿½
                    2. É¾ï¿½ï¿½Ô­ï¿½ï¿½ï¿½ï¿½Curï¿½Úµï¿½(pFTn)
                    3. ï¿½ï¿½ï¿½ï¿½Ò²Ó¦ï¿½ï¿½É¾ï¿½ï¿½pFTnï¿½ï¿½
                    4. ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½pFTnNewï¿½ï¿½
                    5. CurÓ¦ï¿½ï¿½Ö¸ï¿½ï¿½ï¿½ï¿½Ò»ï¿½ï¿½ï¿½ï¿½ï¿½Â½Úµï¿½ï¿½ï¿½Ô¶ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ó£?ï¿½ï¿½ï¿½Ò?¿½ï¿½ï¿½ï¿½Curï¿½ï¿½Í·ï¿½ï¿½ï¿½ï¿½
                    6. ConquerorÓ¦ï¿½ï¿½ï¿½ï¿½Í·ï¿½ï¿½Ê¼ï¿½ï¿½ï¿½ï¿½ÎªCurï¿½ä»¯ï¿½ï¿½
                        |-------|-------------|-------|
                        ^       ^             ^       ^
                    Conqueror  Cur       Conqueror  Cur
                */
                case 3:
                    if(pFTnNew == LW_NULL){
                        printf("something wrong with [fragtree fixup] CASE 3 when fix overlay of nodes containing %d and %d \n", 
                                FT_GET_KEY(pFTn), FT_GET_KEY(pFTnConqueror));
                        return;
                    }
                    else {
                        pFTlistNodeNew = newFragTreeListNode(pFTnNew);
                        hoitFragTreeListInsertNode(pFTlistHeader, pFTlistNodeNew);                  /* ï¿½ï¿½ï¿½ï¿½Úµï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿? */
                        
                        pFTlistNext = pFTlistCur->pFTlistNext;
                        hoitFragTreeListDeleteNode(pFTlistHeader, pFTlistCur);                    /* É¾ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Úµï¿? */
                        lib_free(pFTlistCur);

                        pFTlistCur = pFTlistNext;                                                 /* ï¿½ï¿½ï¿½ï¿½Ç°OuterÎªï¿½ï¿½Ò»ï¿½ï¿½ */
                        pFTlistConqueror = pFTlistHeader->pFTlistHeader;                                /* ï¿½ï¿½ï¿½ï¿½InnerÖ¸ï¿½ë£¬ï¿½ï¿½Îªï¿½ï¿½ï¿½Çµï¿½OuterÒ²ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ */
                        
                        pFTlistConqueror = pFTlistConqueror->pFTlistNext;
                    }
                    break;
                /*! ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ì£ï¿½
                    1. É¾ï¿½ï¿½Cur
                    2. É¾ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ðµï¿½Î»ï¿½Ã£ï¿½
                    3. ï¿½ï¿½ï¿½ï¿½CurÎªï¿½ï¿½Ò»ï¿½ï¿½ï¿½Úµã£»
                    4. ï¿½ï¿½ï¿½ï¿½Conquerorï¿½ï¿½ï¿½ï¿½ÎªCurï¿½ä»¯ï¿½Ë£ï¿½
                        |-------|-------------|-------|
                        ^       ^             ^       ^
                    Conqueror  Cur          Cur  Conqueror
                */
                case 4:
                    pFTlistNext = pFTlistCur->pFTlistNext;
                    hoitFragTreeListDeleteNode(pFTlistHeader, pFTlistCur);                        /* É¾ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Úµï¿? */
                    lib_free(pFTlistCur);

                    pFTlistCur = pFTlistNext;                                                     /* ï¿½ï¿½ï¿½ï¿½Ç°OuterÎªï¿½ï¿½Ò»ï¿½ï¿½ */
                    pFTlistConqueror = pFTlistHeader->pFTlistHeader;                                    /* ï¿½ï¿½ï¿½ï¿½InnerÖ¸ï¿½ë£¬ï¿½ï¿½Îªï¿½ï¿½ï¿½Çµï¿½OuterÒ²ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ */
                    
                    pFTlistConqueror = pFTlistConqueror->pFTlistNext;
                    break;
                default:
                    break;
                }
            }
            else{
                pFTlistConqueror = pFTlistConqueror->pFTlistNext;
            }
        }
        pFTlistCur = pFTlistCur->pFTlistNext;   
    }
    hoitFragTreeListFree(pFTlistHeader);
    return LW_TRUE;
}


#ifdef FT_TEST
VOID hoitFTTreeTest(){
    INT                                 i;
    PHOIT_FRAG_TREE                     pFTTree;
    PHOIT_FRAG_TREE_NODE                pFTn;
    PHOIT_FRAG_TREE_LIST_HEADER         pFTlistHeader;
    PHOIT_FRAG_TREE_LIST_NODE           pFTlistNode;
    BOOL                                res;
    PHOIT_VOLUME                        pfs;
    INT testArray[10] = {8,11,14,15,1,2,4,5,7, 1};

    pfs = (PHOIT_VOLUME)lib_malloc(sizeof(HOIT_VOLUME));
    pFTTree = hoitInitFragTree(pfs);
    
    for (i = 0; i < 10; i++)
    {
        pFTn = newHoitFragTreeNode(LW_NULL, i, 10, testArray[i]);
        hoitFragTreeInsertNode(pFTTree, pFTn);
    }

    /*!  ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ë£ï¿½ï¿½ï¿½ï¿½ï¿½Ë³ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ä»ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ -- SOlVE */
    // INT randomArray[5] = {2,4,5,7, 8};
    // for (i = 0; i < 5; i++)
    // {
    //     printf("Delete Node %d \n", randomArray[i]);
    //     pFTn = hoitFragTreeSearchNode(pFTTree, randomArray[i]);
    //     hoitFragTreeDeleteNode(pFTTree, pFTn);
    // }
    // printf("total nodes: %d \n", pFTTree->uiNCnt);

    pFTn = hoitFragTreeSearchNode(pFTTree, 7);
    printf("pFTn - uiOfs : %d\n", pFTn->uiOfs);
    printf("pFTn - uiSize: %d\n", pFTn->uiSize);
    printf("pFTn - iKey  : %d\n", FT_GET_KEY(pFTn));
    
    printf("\n 1. [test traverse] \n");
    hoitFragTreeTraverse(pFTTree, (PHOIT_FRAG_TREE_NODE)pFTTree->pRbTree->pRbnRoot);
    printf("\n 2. [test delete 7] \n");
    hoitFragTreeDeleteNode(pFTTree, pFTn, LW_FALSE);
    hoitFragTreeTraverse(pFTTree, (PHOIT_FRAG_TREE_NODE)pFTTree->pRbTree->pRbnRoot);
    
    

    printf("\n 3. [test collect range [2, 4] ] \n");
    pFTlistHeader = hoitFragTreeCollectRange(pFTTree, INT_MIN, INT_MAX);
    pFTlistNode = pFTlistHeader->pFTlistHeader->pFTlistNext;
    while (pFTlistNode)
    {
        printf("Key: %d\n", FT_GET_KEY(pFTlistNode->pFTn));   
        pFTlistNode = pFTlistNode->pFTlistNext;
    }
    printf("range [%d, %d] \n", pFTlistHeader->uiLowBound, pFTlistHeader->uiHighBound);
    hoitFragTreeListFree(pFTlistHeader);
    
    printf("\n 4. [test delete range] \n");
    res = hoitFragTreeDeleteRange(pFTTree, 2, 8, LW_FALSE);
    hoitFragTreeTraverse(pFTTree, (PHOIT_FRAG_TREE_NODE)pFTTree->pRbTree->pRbnRoot);
    printf("total nodes: %d\n", pFTTree->uiNCnt);

    printf("\n 5. [test delete FTtree] \n");
    res = hoitFragTreeDeleteTree(pFTTree, LW_FALSE);
}
#endif // DEBUG
