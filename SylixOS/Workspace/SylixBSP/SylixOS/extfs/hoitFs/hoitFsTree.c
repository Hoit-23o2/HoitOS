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
** ��   ��   ��: hoitFsTree.h
**
** ��   ��   ��: Pan yanqi (������)
**
** �ļ���������: 2021 �� 03 �� 28 ��
**
** ��        ��: JFFS2-Like fragtreeʵ��
*********************************************************************************************************/

#include "hoitFsTree.h"
#include "hoitFsCache.h"

#ifdef FT_TEST
PHOIT_FULL_DNODE __hoit_truncate_full_dnode(PHOIT_VOLUME pfs, PHOIT_FULL_DNODE pFDnode, UINT uiOffset, UINT uiSize){
    PHOIT_FULL_DNODE pFDNode = (PHOIT_FULL_DNODE)lib_malloc(sizeof(HOIT_FULL_DNODE));
    return pFDNode;
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
** ��������: __hoitFragTreeCollectRangeHelper
** ��������: ����������ռ�iKeyLow��iKeyHigh֮��Ľڵ�
** �䡡��  : pFTlistHeader          ����ͷ
**            pFTTree               FragTree
**            pFTnRoot              ���ڵ�
**            iKeyLow
** �䡡��  : ����FragTree
** ȫ�ֱ���:
** ����ģ��:
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
        Case 1:         [key1    iKeyLow    key2 ...]             �½磺key1
        Case 2:         [key1    iKeyLow]                         �½磺key1
    */
    if((bHasSuccessor && (FT_GET_KEY(pFTnRoot) <= iKeyLow && FT_GET_KEY(pFTSuccessor) > iKeyLow)) ||
       (!bHasSuccessor && FT_GET_KEY(pFTnRoot) <= iKeyLow)){                                          /* iKey < iKeyLow �� iKey�ĺ�̽ڵ��iKey > iKeyLow*/
        bIsInRange = LW_TRUE;
        pFTlistHeader->uiLowBound = FT_GET_KEY(pFTnRoot);
    }
    /* 
        Case 3:         [iKeyLow   key1   key2 ...]               �½磺key1
        --------------------------------------------------------------------
        Case 1:         [iKeyLow   key1   iKeyHigh]               �Ͻ磺key1
        Case 2:         [iKeyLow   key1   iKeyHigh    key2]       �Ͻ磺key2
        Case 3:         [ikey      iKeyLow   iKeyHigh]            �Ͻ磺key1
    */
    else if (FT_GET_KEY(pFTnRoot) > iKeyLow && FT_GET_KEY(pFTnRoot) <= iKeyHigh){                     /* iKey > iKeyLow �� iKey < iKeyHigh */
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
    if (FT_GET_KEY(pFTnRoot) > iKeyHigh)                                                /* ��֦ */
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
** ��������: hoitInitFragTree
** ��������: ��ʼ��FragTree
** �䡡��  : None
** �䡡��  : ����FragTree
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: hoitFragTreeInsertNode
** ��������: ��FragTree�в���ڵ�
** �䡡��  : pFTTree            FragTree
**           pFTn               ������Ľڵ�   
** �䡡��  : ���ز���Ľڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_FRAG_TREE_NODE hoitFragTreeInsertNode(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn){
    hoitRbInsertNode(pFTTree->pRbTree, &pFTn->pRbn);
    pFTTree->uiNCnt++;
    return pFTn;
}
/*********************************************************************************************************
** ��������: hoitFragTreeSearchNode
** ��������: ��FragTree��������ֵΪiKey�Ľڵ�
** �䡡��  : pFTTree          FragTree
**           iKey             ��ֵ
** �䡡��  : �ҵ��ͷ���FragTree�ڵ㣬���򷵻�LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_FRAG_TREE_NODE hoitFragTreeSearchNode(PHOIT_FRAG_TREE pFTTree, INT32 iKey){
    PHOIT_RB_NODE           pRbn; 
    PHOIT_FRAG_TREE_NODE    pFTn;

    pRbn = hoitRbSearchNode(pFTTree->pRbTree, iKey);
    pFTn = (PHOIT_FRAG_TREE_NODE)(pRbn);  
    return pFTn;
}
/*********************************************************************************************************
** ��������: hoitFragTreeSearchNode
** ��������: ��FragTree��������ֵ�ڷ�Χ[x, y]�Ľڵ㣬���У�x <= iKeyLow��y >= iKeyHigh����������
** �䡡��  : pFTTree          FragTree
**           iKeyLow          �ͼ�ֵ
**           iKeyHigh         �߼�ֵ  
** �䡡��  : ��������ͷ
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: hoitFragTreeDeleteNode
** ��������: ��FragTree��ɾ��ĳ���ڵ㣬��ɾ����ָ���FullDnode�ڵ㣬���ͷ����ڴ棬ע�Ᵽ��
** �䡡��  : pFTTree          FragTree
**           pFTn             FragTree�ϵ�ĳ���ڵ�
** �䡡��  : ɾ���ɹ�����True�����򷵻�False
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL hoitFragTreeDeleteNode(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn, BOOL bDoDelete){
    BOOL        res; 
    if(pFTTree->uiNCnt == 0)
        return LW_FALSE;
    
    res = hoitRbDeleteNode(pFTTree->pRbTree, &pFTn->pRbn);
    if(res){
        pFTTree->uiNCnt--;
        __hoit_delete_full_dnode(pFTTree->pfs, pFTn->pFDnode, bDoDelete); /* ɾ��Dnodeָ����ڴ�ռ� */
        lib_free(pFTn);                                                   /* ɾ��pFTnָ����ڴ�ռ� */
    }
    return res;
}
/*********************************************************************************************************
** ��������: hoitFragTreeDeleteNode
** ��������: ��FragTree��ɾ��ĳ��Χ�ڵĽڵ�
** �䡡��  : pFTTree          FragTree
**           iKeyLow          �ͼ�ֵ
**           iKeyHigh         �߼�ֵ
** �䡡��  : ɾ���ɹ�����True�����򷵻�False
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL hoitFragTreeDeleteRange(PHOIT_FRAG_TREE pFTTree, INT32 iKeyLow, INT32 iKeyHigh, BOOL bDoDelete){
    PHOIT_FRAG_TREE_LIST_HEADER pFTlistHeader;
    PHOIT_FRAG_TREE_LIST_NODE   pFTlistNode;
    BOOL                        res;

    pFTlistHeader = hoitFragTreeCollectRange(pFTTree, iKeyLow, iKeyHigh);
    pFTlistNode = pFTlistHeader->pFTlistHeader->pFTlistNext;
    INT count = 0;
    while (pFTlistNode)
    {
        count++;
#ifdef FT_DEBUG
        printf("count %d\n", count);
#endif
        if(count == 1){
            pFTlistNode->pFTn->uiOfs;    
        }
        if(count == pFTlistHeader->uiNCnt){

        }
        
        res = hoitFragTreeDeleteNode(pFTTree, pFTlistNode->pFTn, bDoDelete);
        
        pFTlistNode = pFTlistNode->pFTlistNext;
        if(res == LW_FALSE){
            break;
        }
    }
    hoitFragTreeListFree(pFTlistHeader);
    return res;    
}

/*********************************************************************************************************
** ��������: hoitFragTreeDeleteTree
** ��������: ɾ������FragTree�������ͷ�pFTTree���ڴ�
** �䡡��  : pFTTree          FragTree
** �䡡��  : ɾ���ɹ�����True�����򷵻�False
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: hoitFragTreeTraverse
** ��������: �������FragTree
** �䡡��  : pFTTree          FragTree
**           pFTnRoot         FragTree������
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: hoitFragTreeRead
** ��������: �������FragTree
** �䡡��  : pFTTree          FragTree
**           uiOfs            ����ļ���ƫ��
**           uiSize           ����
**           pContent         ��������λ��
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
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
        /* ���׸�����ʵ��
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
        /* �����һ������ʵ��
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
        else                                                                        /* ������� */
        {
            uiPerOfs = uiPhySize;
            uiPerSize = uiPhySize;
        }

        pPerContent = (PCHAR)lib_malloc(uiPerSize);                                 /* �������� */
        //TODO: �ӻ����϶� 
        hoitReadFromCache(uiPerOfs, pPerContent, uiPerSize);

        lib_memcpy(pContent + uiSizeRead, pPerContent, uiPerSize);
        lib_free(pPerContent);
        
        pFTlist = pFTlist->pFTlistNext;
        uiSizeRead += uiPerSize;
        uiSizeRemain -= uiPerSize;
    }

    hoitFragTreeListFree(pFTlistHeader);                                             /* �ͷ����� */
    return ERROR_NONE;
}
/*********************************************************************************************************
** ��������: hoitFragTreeOverlayFixUp
** ��������: �޸�FragTree���ص��Ľڵ�
** �䡡��  : pFTTree          FragTree
** �䡡��  : ���޸����LW_TRUE���������LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL hoitFragTreeOverlayFixUp(PHOIT_FRAG_TREE pFTTree){
    PHOIT_FRAG_TREE_LIST_HEADER     pFTlistHeader;
    PHOIT_FRAG_TREE_LIST_NODE       pFTlistOuter;
    PHOIT_FRAG_TREE_LIST_NODE       pFTlistInner;
    PHOIT_FRAG_TREE_LIST_NODE       pFTlistNext;

    PHOIT_FULL_DNODE                pFDNodeNew;
    PHOIT_FRAG_TREE_NODE            pFTnNew;
    PHOIT_FRAG_TREE_LIST_NODE       pFTlistNodeNew;
    
    UINT32                          uiCurLow;                                 /* ���䣺[curLow, curHigh) */
    UINT32                          uiCurHigh;

    UINT32                          uiConquerorLow;
    UINT32                          uiConquerorHigh;
    
    UINT32                          uiLeftRemainSize;
    UINT32                          uiRightRemainSize;
    UINT32                          uiRightOffset;

    BOOL                            bIsOverlay;

    pFTlistHeader = hoitFragTreeCollectRange(pFTTree, INT_MIN, INT_MAX);
    pFTlistOuter = pFTlistHeader->pFTlistHeader->pFTlistNext;
    
    while (pFTlistOuter != LW_NULL)                     /* ���Ϊ��ɫ���ڲ�Ϊ��ɫ */
    {
        uiCurLow = pFTlistOuter->pFTn->uiOfs;
        uiCurHigh = uiCurLow + pFTlistOuter->pFTn->uiSize;
        //TODO: �����￪ʼ����ص� 
        pFTlistInner = pFTlistHeader->pFTlistHeader->pFTlistNext;               /* ָ���һ����Ч�ڵ� */
        while (pFTlistInner != LW_NULL)
        {
            bIsOverlay = LW_FALSE;
            if(pFTlistInner == pFTlistOuter)                                    /* �Լ��Ͳ����Լ��Ƚ��� */
            {
                continue;
            }
            if(pFTlistInner->pFTn->pFDnode->HOITFD_version                      /* listInner���������ߣ�������version������ڱ������߲��ܽ������� */
             < pFTlistOuter->pFTn->pFDnode->HOITFD_version){
                continue;
            }
            uiConquerorLow = pFTlistInner->pFTn->uiOfs;
            uiConquerorHigh = uiConquerorLow + pFTlistInner->pFTn->uiSize;
            
            bIsOverlay = MAX(uiCurLow, uiConquerorLow) <= MIN(uiCurHigh, uiConquerorHigh);
            if(bIsOverlay){                                                               /* ���4������޸�����ɫΪcur����ɫΪtarget */
                /* 
                    |-------|-------------|-------|
                    ^       ^             ^       ^
                    Cur  Conqueror       Cur    Conqueror  
                */
                if(uiCurLow < uiConquerorLow && uiCurHigh <= uiConquerorHigh){            /* ���1 */      
                    uiLeftRemainSize = uiConquerorLow - uiCurLow;
                    pFTlistOuter->pFTn->uiSize = uiLeftRemainSize;
                    pFTlistOuter->pFTn->pFDnode->HOITFD_length = uiLeftRemainSize;
                    
                    pFTlistInner = pFTlistInner->pFTlistNext;
                }
                /* 
                    |-------|-------------|-------|
                    ^       ^             ^       ^
                    Cur  Conqueror      Conqueror  Cur
                */
                else if (uiCurLow < uiConquerorLow && uiCurHigh > uiConquerorHigh)        /* ���2 */  
                {
                    uiLeftRemainSize = uiCurLow - uiConquerorLow;
                    uiRightRemainSize = uiConquerorHigh - uiCurHigh;
                    uiRightOffset = uiConquerorHigh - uiCurLow;

                    pFDNodeNew = __hoit_truncate_full_dnode(pFTTree->pfs,                        /* �ض��ұ߲��֣������µĽڵ� */
                                                           pFTlistOuter->pFTn->pFDnode,
                                                           uiRightOffset,
                                                           uiRightRemainSize);

                    pFTlistOuter->pFTn->uiSize = uiLeftRemainSize;                              /* �޸���߲��ֵ�size */
                    pFTlistOuter->pFTn->pFDnode->HOITFD_length = uiLeftRemainSize;

                    pFTnNew = newHoitFragTreeNode(pFDNodeNew, pFDNodeNew->HOITFD_length,        /* ����pFDNodeNew�½�FragTreeNode */
                                                  pFDNodeNew->HOITFD_offset, pFDNodeNew->HOITFD_offset);

                    hoitFragTreeInsertNode(pFTTree, pFTnNew);                                   /* ����ڵ�������� */
                    pFTlistNodeNew = newFragTreeListNode(pFTnNew);
                    hoitFragTreeListInsertNode(pFTlistHeader, pFTlistNodeNew);                  /* ����ڵ������� */

                    pFTlistInner = pFTlistInner->pFTlistNext;
                }
                /* 
                        |-------|-------------|-------|
                        ^       ^             ^       ^
                    Conqueror  Cur       Conqueror  Cur
                */
                else if (uiCurLow >= uiConquerorLow && uiCurHigh > uiConquerorHigh)       /* ���3 */  
                {
                    uiRightRemainSize = uiCurHigh - uiConquerorHigh;
                    uiRightOffset = uiConquerorHigh - uiCurLow;
                    pFDNodeNew = __hoit_truncate_full_dnode(pFTTree->pfs,
                                                           pFTlistOuter->pFTn->pFDnode,
                                                           uiRightOffset,
                                                           uiRightRemainSize);
                    pFTnNew = newHoitFragTreeNode(pFDNodeNew, pFDNodeNew->HOITFD_length,        /* ����pFDNodeNew�½�FragTreeNode */
                                                  pFDNodeNew->HOITFD_offset, pFDNodeNew->HOITFD_offset);
                    
                    
                    hoitFragTreeInsertNode(pFTTree, pFTnNew);
                    pFTlistNodeNew = newFragTreeListNode(pFTnNew);
                    hoitFragTreeListInsertNode(pFTlistHeader, pFTlistNodeNew);                  /* ����ڵ������� */


                    hoitFragTreeDeleteNode(pFTTree, pFTlistOuter->pFTn, LW_TRUE);               /* ɾ��������ڵ� */
                    pFTlistNext = pFTlistOuter->pFTlistNext;
                    hoitFragTreeListDeleteNode(pFTlistHeader, pFTlistOuter);                    /* ɾ������ڵ� */
                    lib_free(pFTlistOuter);

                    pFTlistOuter = pFTlistNext;                                                 /* ����ǰOuterΪ��һ�� */
                    pFTlistInner = pFTlistHeader->pFTlistHeader->pFTlistNext;                   /* ����Innerָ�룬��Ϊ���ǵ�OuterҲ������ */
                }
                /* 
                    |-------|-------------|-------|
                    ^       ^             ^       ^
                   Cur  Conqueror     Conqueror  Cur
                */
                else if (uiCurLow <= uiConquerorLow && uiCurHigh >= uiConquerorHigh)            /* ���4 */  
                {
                    hoitFragTreeDeleteNode(pFTTree, pFTlistInner->pFTn, LW_TRUE);               /* ɾ����ǰFragTree�ϵĽڵ� */
                    pFTlistNext = pFTlistInner->pFTlistNext;
                    hoitFragTreeListDeleteNode(pFTlistHeader, pFTlistInner);
                    lib_free(pFTlistInner);
                    
                    pFTlistInner = pFTlistNext;                                                 /* ɾ���ڵ㲻��Ӱ�������ڵ�ĸ��ǹ�ϵ�����ֱ�Ӽ����һ���ڵ� */
                }
            }
            else {
                pFTlistInner = pFTlistInner->pFTlistNext;
            }
        }
        pFTlistOuter = pFTlistOuter->pFTlistNext;   
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

    /*!  ���������ˣ�����˳�������Ļ��������� -- SOlVE */
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