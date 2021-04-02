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

/*********************************************************************************************************
** 函数名称: hoitInitFragTree
** 功能描述: 初始化FragTree
** 输　入  : None
** 输　出  : 返回FragTree
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_FRAG_TREE hoitInitFragTree(VOID){
    PHOIT_FRAG_TREE pFTTree = (PHOIT_FRAG_TREE)lib_malloc(sizeof(HOIT_FRAG_TREE));
    pFTTree->pRbTree = hoitRbInitTree();
    pFTTree->uiNCnt = 0;
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
    PHOIT_RB_NODE pRbn = hoitRbSearchNode(pFTTree->pRbTree, iKey);
    PHOIT_FRAG_TREE_NODE pFTn = (PHOIT_FRAG_TREE_NODE)(pRbn);  
    return pFTn;
}
/*********************************************************************************************************
** 函数名称: hoitFragTreeDeleteNode
** 功能描述: 在FragTree上删除某个节点，并释放其内存，注意保存
** 输　入  : pFTTree          FragTree
**           pFTn             FragTree上的某个节点
** 输　出  : 删除成功返回True，否则返回False
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL hoitFragTreeDeleteNode(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn){
    if(pFTTree->uiNCnt == 0)
        return LW_FALSE;
    pFTTree->uiNCnt--;
    BOOL res = hoitRbDeleteNode(pFTTree->pRbTree, &pFTn->pRbn);
    lib_free(pFTn);
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
    if(&pFTnRoot->pRbn == pFTTree->pRbTree->pRbnGuard){
        return;
    }
    else
    {
        hoitFragTreeTraverse(pFTTree, (PHOIT_FRAG_TREE_NODE) pFTnRoot->pRbn.pRbnLeft);
        printf("uiOfs: %d, uiSize: %d, iKey: %d \n", pFTnRoot->uiOfs, pFTnRoot->uiSize, pFTnRoot->pRbn.iKey);
        hoitFragTreeTraverse(pFTTree, (PHOIT_FRAG_TREE_NODE) pFTnRoot->pRbn.pRbnRight);
    }
}

#ifdef FT_TEST
VOID hoitFTTreeTest(){
    INT i;
    PHOIT_FRAG_TREE pFTTree;
    PHOIT_FRAG_TREE_NODE pFTn;
    INT testArray[10] = {8,11,14,15,1,2,4,5,7, 1};

    pFTTree = hoitInitFragTree();
    
    for (i = 0; i < 10; i++)
    {
        pFTn = newHoitFragTreeNode(LW_NULL, i, 10, testArray[i]);
        hoitFragTreeInsertNode(pFTTree, pFTn);
    }

    pFTn = hoitFragTreeSearchNode(pFTTree, 7);
    printf("pFTn - uiOfs : %d\n", pFTn->uiOfs);
    printf("pFTn - uiSize: %d\n", pFTn->uiSize);
    printf("pFTn - iKey  : %d\n", pFTn->pRbn.iKey);

    printf("[test traverse] \n");
    hoitFragTreeTraverse(pFTTree, (PHOIT_FRAG_TREE_NODE)pFTTree->pRbTree->pRbnRoot);
    printf("[test delete 7] \n");
    hoitFragTreeDeleteNode(pFTTree, pFTn);
    hoitFragTreeTraverse(pFTTree, (PHOIT_FRAG_TREE_NODE)pFTTree->pRbTree->pRbnRoot);
}
#endif // DEBUG