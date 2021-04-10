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
** 文   件   名: hoitFsTree.h
**
** 创   建   人: Pan yanqi (潘延麒)
**
** 文件创建日期: 2021 年 03 月 28 日
**
** 描        述: JFFS2-Like fragtree实现
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
** 函数名称: __hoitFragTreeCollectRangeHelper
** 功能描述: 中序遍历，收集iKeyLow到iKeyHigh之间的节点
** 输　入  : pFTlistHeader          链表头
**            pFTTree               FragTree
**            pFTnRoot              根节点
**            iKeyLow
** 输　出  : 返回FragTree
** 全局变量:
** 调用模块:
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
        Case 1:         [key1    iKeyLow    key2 ...]             下界：key1
        Case 2:         [key1    iKeyLow]                         下界：key1
    */
    if((bHasSuccessor && (FT_GET_KEY(pFTnRoot) <= iKeyLow && FT_GET_KEY(pFTSuccessor) > iKeyLow)) ||
       (!bHasSuccessor && FT_GET_KEY(pFTnRoot) <= iKeyLow)){                                          /* iKey < iKeyLow 且 iKey的后继节点的iKey > iKeyLow*/
        bIsInRange = LW_TRUE;
        pFTlistHeader->uiLowBound = FT_GET_KEY(pFTnRoot);
    }
    /* 
        Case 3:         [iKeyLow   key1   key2 ...]               下界：key1
        --------------------------------------------------------------------
        Case 1:         [iKeyLow   key1   iKeyHigh]               上界：key1
        Case 2:         [iKeyLow   key1   iKeyHigh    key2]       上界：key2
        Case 3:         [ikey      iKeyLow   iKeyHigh]            上界：key1
    */
    else if (FT_GET_KEY(pFTnRoot) > iKeyLow && FT_GET_KEY(pFTnRoot) <= iKeyHigh){                     /* iKey > iKeyLow 且 iKey < iKeyHigh */
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
    if (FT_GET_KEY(pFTnRoot) > iKeyHigh)                                                /* 剪枝 */
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
** 函数名称: hoitInitFragTree
** 功能描述: 初始化FragTree
** 输　入  : None
** 输　出  : 返回FragTree
** 全局变量:
** 调用模块:
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
** 函数名称: hoitFragTreeInsertNode
** 功能描述: 向FragTree中插入节点
** 输　入  : pFTTree            FragTree
**           pFTn               待插入的节点   
** 输　出  : 返回插入的节点
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_FRAG_TREE_NODE hoitFragTreeInsertNode(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn){
    hoitRbInsertNode(pFTTree->pRbTree, &pFTn->pRbn);
    pFTTree->uiNCnt++;
    return pFTn;
}
/*********************************************************************************************************
** 函数名称: hoitFragTreeSearchNode
** 功能描述: 在FragTree上搜索键值为iKey的节点
** 输　入  : pFTTree          FragTree
**           iKey             键值
** 输　出  : 找到就返回FragTree节点，否则返回LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_FRAG_TREE_NODE hoitFragTreeSearchNode(PHOIT_FRAG_TREE pFTTree, INT32 iKey){
    PHOIT_RB_NODE           pRbn; 
    PHOIT_FRAG_TREE_NODE    pFTn;

    pRbn = hoitRbSearchNode(pFTTree->pRbTree, iKey);
    pFTn = (PHOIT_FRAG_TREE_NODE)(pRbn);  
    return pFTn;
}
/*********************************************************************************************************
** 函数名称: hoitFragTreeSearchNode
** 功能描述: 在FragTree上搜索键值在范围[x, y]的节点，其中，x <= iKeyLow，y >= iKeyHigh，返回链表
** 输　入  : pFTTree          FragTree
**           iKeyLow          低键值
**           iKeyHigh         高键值  
** 输　出  : 返回链表头
** 全局变量:
** 调用模块:
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
** 函数名称: hoitFragTreeDeleteNode
** 功能描述: 在FragTree上删除某个节点，并删除其指向的FullDnode节点，并释放其内存，注意保存
** 输　入  : pFTTree          FragTree
**           pFTn             FragTree上的某个节点
** 输　出  : 删除成功返回True，否则返回False
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL hoitFragTreeDeleteNode(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn, BOOL bDoDelete){
    BOOL        res; 
    if(pFTTree->uiNCnt == 0)
        return LW_FALSE;
    
    res = hoitRbDeleteNode(pFTTree->pRbTree, &pFTn->pRbn);
    if(res){
        pFTTree->uiNCnt--;
        __hoit_delete_full_dnode(pFTTree->pfs, pFTn->pFDnode, bDoDelete); /* 删除Dnode指向的内存空间 */
        lib_free(pFTn);                                                   /* 删除pFTn指向的内存空间 */
    }
    return res;
}
/*********************************************************************************************************
** 函数名称: hoitFragTreeDeleteNode
** 功能描述: 在FragTree上删除某范围内的节点
** 输　入  : pFTTree          FragTree
**           iKeyLow          低键值
**           iKeyHigh         高键值
** 输　出  : 删除成功返回True，否则返回False
** 全局变量:
** 调用模块:
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
** 函数名称: hoitFragTreeDeleteTree
** 功能描述: 删除整个FragTree，并且释放pFTTree的内存
** 输　入  : pFTTree          FragTree
** 输　出  : 删除成功返回True，否则返回False
** 全局变量:
** 调用模块:
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
** 函数名称: hoitFragTreeTraverse
** 功能描述: 中序遍历FragTree
** 输　入  : pFTTree          FragTree
**           pFTnRoot         FragTree子树根
** 输　出  : None
** 全局变量:
** 调用模块:
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
** 函数名称: hoitFragTreeRead
** 功能描述: 中序遍历FragTree
** 输　入  : pFTTree          FragTree
**           uiOfs            相对文件的偏移
**           uiSize           长度
**           pContent         读出数据位置
** 输　出  : None
** 全局变量:
** 调用模块:
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
        /* 读首个数据实体
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
        /* 读最后一个数据实体
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
        else                                                                        /* 其他情况 */
        {
            uiPerOfs = uiPhySize;
            uiPerSize = uiPhySize;
        }

        pPerContent = (PCHAR)lib_malloc(uiPerSize);                                 /* 读入数据 */
        //TODO: 从缓存上读 
        hoitReadFromCache(uiPerOfs, pPerContent, uiPerSize);

        lib_memcpy(pContent + uiSizeRead, pPerContent, uiPerSize);
        lib_free(pPerContent);
        
        pFTlist = pFTlist->pFTlistNext;
        uiSizeRead += uiPerSize;
        uiSizeRemain -= uiPerSize;
    }

    hoitFragTreeListFree(pFTlistHeader);                                             /* 释放链表 */
    return ERROR_NONE;
}
/*********************************************************************************************************
** 函数名称: hoitFragTreeOverlayFixUp
** 功能描述: 修复FragTree上重叠的节点
** 输　入  : pFTTree          FragTree
** 输　出  : 有修复输出LW_TRUE，否则输出LW_FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL hoitFragTreeOverlayFixUp(PHOIT_FRAG_TREE pFTTree){
    PHOIT_FRAG_TREE_LIST_HEADER     pFTlistHeader;
    PHOIT_FRAG_TREE_LIST_NODE       pFTlistOuter;
    PHOIT_FRAG_TREE_LIST_NODE       pFTlistInner;
    PHOIT_FRAG_TREE_LIST_NODE       pFTlistNext;

    PHOIT_FULL_DNODE                pFDNodeNew;
    PHOIT_FRAG_TREE_NODE            pFTnNew;
    PHOIT_FRAG_TREE_LIST_NODE       pFTlistNodeNew;
    
    UINT32                          uiCurLow;                                 /* 区间：[curLow, curHigh) */
    UINT32                          uiCurHigh;

    UINT32                          uiConquerorLow;
    UINT32                          uiConquerorHigh;
    
    UINT32                          uiLeftRemainSize;
    UINT32                          uiRightRemainSize;
    UINT32                          uiRightOffset;

    BOOL                            bIsOverlay;

    pFTlistHeader = hoitFragTreeCollectRange(pFTTree, INT_MIN, INT_MAX);
    pFTlistOuter = pFTlistHeader->pFTlistHeader->pFTlistNext;
    
    while (pFTlistOuter != LW_NULL)                     /* 外层为紫色，内层为灰色 */
    {
        uiCurLow = pFTlistOuter->pFTn->uiOfs;
        uiCurHigh = uiCurLow + pFTlistOuter->pFTn->uiSize;
        //TODO: 从这里开始检测重叠 
        pFTlistInner = pFTlistHeader->pFTlistHeader->pFTlistNext;               /* 指向第一个有效节点 */
        while (pFTlistInner != LW_NULL)
        {
            bIsOverlay = LW_FALSE;
            if(pFTlistInner == pFTlistOuter)                                    /* 自己就不和自己比较了 */
            {
                continue;
            }
            if(pFTlistInner->pFTn->pFDnode->HOITFD_version                      /* listInner代表征服者，征服者version必须大于被征服者才能进行征服 */
             < pFTlistOuter->pFTn->pFDnode->HOITFD_version){
                continue;
            }
            uiConquerorLow = pFTlistInner->pFTn->uiOfs;
            uiConquerorHigh = uiConquerorLow + pFTlistInner->pFTn->uiSize;
            
            bIsOverlay = MAX(uiCurLow, uiConquerorLow) <= MIN(uiCurHigh, uiConquerorHigh);
            if(bIsOverlay){                                                               /* 针对4种情况修复，紫色为cur，灰色为target */
                /* 
                    |-------|-------------|-------|
                    ^       ^             ^       ^
                    Cur  Conqueror       Cur    Conqueror  
                */
                if(uiCurLow < uiConquerorLow && uiCurHigh <= uiConquerorHigh){            /* 情况1 */      
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
                else if (uiCurLow < uiConquerorLow && uiCurHigh > uiConquerorHigh)        /* 情况2 */  
                {
                    uiLeftRemainSize = uiCurLow - uiConquerorLow;
                    uiRightRemainSize = uiConquerorHigh - uiCurHigh;
                    uiRightOffset = uiConquerorHigh - uiCurLow;

                    pFDNodeNew = __hoit_truncate_full_dnode(pFTTree->pfs,                        /* 截断右边部分，生成新的节点 */
                                                           pFTlistOuter->pFTn->pFDnode,
                                                           uiRightOffset,
                                                           uiRightRemainSize);

                    pFTlistOuter->pFTn->uiSize = uiLeftRemainSize;                              /* 修改左边部分的size */
                    pFTlistOuter->pFTn->pFDnode->HOITFD_length = uiLeftRemainSize;

                    pFTnNew = newHoitFragTreeNode(pFDNodeNew, pFDNodeNew->HOITFD_length,        /* 根据pFDNodeNew新建FragTreeNode */
                                                  pFDNodeNew->HOITFD_offset, pFDNodeNew->HOITFD_offset);

                    hoitFragTreeInsertNode(pFTTree, pFTnNew);                                   /* 插入节点至红黑树 */
                    pFTlistNodeNew = newFragTreeListNode(pFTnNew);
                    hoitFragTreeListInsertNode(pFTlistHeader, pFTlistNodeNew);                  /* 插入节点至链表 */

                    pFTlistInner = pFTlistInner->pFTlistNext;
                }
                /* 
                        |-------|-------------|-------|
                        ^       ^             ^       ^
                    Conqueror  Cur       Conqueror  Cur
                */
                else if (uiCurLow >= uiConquerorLow && uiCurHigh > uiConquerorHigh)       /* 情况3 */  
                {
                    uiRightRemainSize = uiCurHigh - uiConquerorHigh;
                    uiRightOffset = uiConquerorHigh - uiCurLow;
                    pFDNodeNew = __hoit_truncate_full_dnode(pFTTree->pfs,
                                                           pFTlistOuter->pFTn->pFDnode,
                                                           uiRightOffset,
                                                           uiRightRemainSize);
                    pFTnNew = newHoitFragTreeNode(pFDNodeNew, pFDNodeNew->HOITFD_length,        /* 根据pFDNodeNew新建FragTreeNode */
                                                  pFDNodeNew->HOITFD_offset, pFDNodeNew->HOITFD_offset);
                    
                    
                    hoitFragTreeInsertNode(pFTTree, pFTnNew);
                    pFTlistNodeNew = newFragTreeListNode(pFTnNew);
                    hoitFragTreeListInsertNode(pFTlistHeader, pFTlistNodeNew);                  /* 插入节点至链表 */


                    hoitFragTreeDeleteNode(pFTTree, pFTlistOuter->pFTn, LW_TRUE);               /* 删除红黑树节点 */
                    pFTlistNext = pFTlistOuter->pFTlistNext;
                    hoitFragTreeListDeleteNode(pFTlistHeader, pFTlistOuter);                    /* 删除链表节点 */
                    lib_free(pFTlistOuter);

                    pFTlistOuter = pFTlistNext;                                                 /* 至当前Outer为下一个 */
                    pFTlistInner = pFTlistHeader->pFTlistHeader->pFTlistNext;                   /* 重置Inner指针，因为我们的Outer也重置了 */
                }
                /* 
                    |-------|-------------|-------|
                    ^       ^             ^       ^
                   Cur  Conqueror     Conqueror  Cur
                */
                else if (uiCurLow <= uiConquerorLow && uiCurHigh >= uiConquerorHigh)            /* 情况4 */  
                {
                    hoitFragTreeDeleteNode(pFTTree, pFTlistInner->pFTn, LW_TRUE);               /* 删除当前FragTree上的节点 */
                    pFTlistNext = pFTlistInner->pFTlistNext;
                    hoitFragTreeListDeleteNode(pFTlistHeader, pFTlistInner);
                    lib_free(pFTlistInner);
                    
                    pFTlistInner = pFTlistNext;                                                 /* 删除节点不会影响其他节点的覆盖关系，因此直接检查下一个节点 */
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

    /*!  出现问题了，按照顺序搜索的话有问题了 -- SOlVE */
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