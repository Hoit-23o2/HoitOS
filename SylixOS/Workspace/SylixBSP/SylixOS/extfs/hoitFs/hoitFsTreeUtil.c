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
** 文   件   名: hoitFsTreeUtil.h
**
** 创   建   人: Pan yanqi (潘延麒)
**
** 文件创建日期: 2021 年 03 月 28 日
**
** 描        述: JFFS2-Like 红黑树数据结构
*********************************************************************************************************/

#include "hoitFsTreeUtil.h"

#define RB_LEFT_CHILD(pRbn)          (pRbn->pRbnLeft)
#define RB_RIGHT_CHILD(pRbn)         (pRbn->pRbnRight)
#define RB_PARENT(pRbn)              (pRbn->pRbnParent)
#define RB_GRAND(pRbn)               RB_PARENT(RB_PARENT(pRbn))
#define RB_UNCLE_RIGHT(pRbn)         (RB_RIGHT_CHILD(RB_GRAND(pRbn)))
#define RB_UNCLE_LEFT(pRbn)          (RB_LEFT_CHILD(RB_GRAND(pRbn)))

#define RB_IS_LEFT_CHILD(pRbn)       pRbn == RB_LEFT_CHILD(RB_PARENT(pRbn))
#define RB_IS_RIGHT_CHILD(pRbn)      pRbn == RB_RIGHT_CHILD(RB_PARENT(pRbn))

PHOIT_RB_NODE __hoitRbMinimum(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbnRoot){
    PHOIT_RB_NODE pRbnTraverse;
    pRbnTraverse = pRbnRoot;
    while (pRbnTraverse != pRbTree->pRbnGuard)
    {
        pRbnTraverse = RB_LEFT_CHILD(pRbnTraverse);   
    }
    return pRbnTraverse;
}

PHOIT_RB_NODE __hoitRbMaximum(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbnRoot){
    PHOIT_RB_NODE pRbnTraverse;
    pRbnTraverse = pRbnRoot;
    while (pRbnTraverse != pRbTree->pRbnGuard)
    {
        pRbnTraverse = RB_IS_RIGHT_CHILD(pRbnTraverse);   
    }
    return pRbnTraverse;
}

VOID __hoitRbTransplant(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbnTarget, PHOIT_RB_NODE pRbnConquer){
    if(RB_PARENT(pRbnTarget) == pRbTree->pRbnGuard){
        pRbTree->pRbnRoot = pRbnConquer;
    }
    else if (RB_IS_LEFT_CHILD(pRbnTarget))
    {
        pRbnTarget->pRbnParent->pRbnLeft = pRbnConquer;
    }
    else pRbnTarget->pRbnParent->pRbnRight = pRbnConquer;

    pRbnConquer->pRbnParent = pRbnTarget->pRbnParent;
}
/*********************************************************************************************************
** 函数名称: __hoitFsLeftRotate
** 功能描述: 红黑树左旋
** 输　入  : pRbTree          红黑树
**           pRbn              红黑树节点
** 输　出  : 成功返回True，失败返回False
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL __hoitRbLeftRotate(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn){
    PHOIT_RB_NODE           pRbnRight;
    
    pRbnRight = pRbn->pRbnRight;
    
    if(pRbnRight == pRbTree->pRbnGuard){
        printf("left rotate: target's right child can't be null\n");
        return FALSE;
    }
    
    pRbn->pRbnRight = RB_LEFT_CHILD(pRbnRight);         /* 移动当前节点右孩子的左孩子至该节点的右节点之下 */
    
    if(pRbnRight->pRbnLeft != pRbTree->pRbnGuard){      /* 设置当前节点右孩子的左孩子节点的父亲节点： 如果该节点是空，则不需设置，否则设置为当前节点*/
        pRbnRight->pRbnLeft->pRbnParent = pRbn;
    }

    pRbnRight->pRbnParent = RB_PARENT(pRbn);            /* 设置当前节点右孩子的父节点 */
    
    if(pRbn->pRbnParent == pRbTree->pRbnGuard){         /* 如果当前节点的父亲为空，则当前节点的右节点为红黑树的根 */
        pRbTree->pRbnRoot = pRbnRight;
    }
    else if(pRbn == pRbn->pRbnParent->pRbnLeft){        /* 当前节点是左孩子 */
        pRbn->pRbnParent->pRbnLeft = pRbnRight;
    }
    else pRbn->pRbnParent->pRbnRight = pRbnRight;       /* 当前节点是右孩子 */
    
    pRbnRight->pRbnLeft = pRbn;
    pRbn->pRbnParent = pRbnRight;

    return TRUE;
}
/*********************************************************************************************************
** 函数名称: __hoitFsRightRotate
** 功能描述: 红黑树右旋
** 输　入  : pRbTree          红黑树
**           pRbn              红黑树节点
** 输　出  : 成功返回True，失败返回False
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL __hoitRbRightRotate(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn){
    PHOIT_RB_NODE           pRbnLeft;
    
    pRbnLeft = pRbn->pRbnLeft;
    
    if(pRbnLeft == pRbTree->pRbnGuard){
        printf("right rotate: target's left child can't be null\n");
        return FALSE;
    }
    
    pRbn->pRbnLeft = RB_RIGHT_CHILD(pRbnLeft);              
    
    if(pRbnLeft->pRbnRight != pRbTree->pRbnGuard){      
        pRbnLeft->pRbnRight->pRbnParent = pRbn;
    }

    pRbnLeft->pRbnParent = RB_PARENT(pRbn);           
    
    if(pRbn->pRbnParent == pRbTree->pRbnGuard){         
        pRbTree->pRbnRoot = pRbnLeft;
    }
    else if(pRbn == pRbn->pRbnParent->pRbnLeft){        /* 当前节点是左孩子 */
        pRbn->pRbnParent->pRbnLeft = pRbnLeft;
    }
    else pRbn->pRbnParent->pRbnRight = pRbnLeft;       /* 当前节点是右孩子 */
    
    pRbnLeft->pRbnRight = pRbn;
    pRbn->pRbnParent = pRbnLeft;

    return TRUE;
}

/*********************************************************************************************************
** 函数名称: __hoitRbInsertFixUp
** 功能描述: 重绘节点颜色
** 输　入  : pRbTree          红黑树
**           pRbn              待插入红黑树节点
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __hoitRbInsertFixUp(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn){
    while (RB_PARENT(pRbn)->uiColor == RB_RED)
    {
        if(RB_IS_LEFT_CHILD(RB_PARENT(pRbn))){
            /* 父亲是左孩子, 叔叔节点为右侧, 且是红色 
                       PP(B)
                      /   \
                   P(R)    uncle(R)         
                   /
                pRbn(R)
                        
                        或

                        PP(B)
                      /   \
                   P(R)    uncle(R)         
                      \
                    pRbn(R)
            */
            if(RB_UNCLE_RIGHT(pRbn)->uiColor == RB_RED){
                RB_PARENT(pRbn)->uiColor = RB_BLACK;
                RB_UNCLE_RIGHT(pRbn)->uiColor = RB_BLACK;
                RB_GRAND(pRbn)->uiColor = RB_RED;
                pRbn = RB_GRAND(pRbn);
            }
            /* 该节点是右孩子，叔叔是黑色
                      PP(B)
                      /   \
                   P(R)    uncle(B)
                      \
                    pRbn(R)
            */
            else if(RB_IS_RIGHT_CHILD(pRbn)){
                pRbn = RB_PARENT(pRbn);
                __hoitRbLeftRotate(pRbTree, pRbn);
            }
            /* 该节点的叔叔是黑色的，且自己是左孩子
                      PP(B)
                      /   \
                   P(R)    uncle(B)
                   /
                pRbn(R)
            */
            if(RB_IS_LEFT_CHILD(pRbn) && RB_UNCLE_RIGHT(pRbn)->uiColor == RB_BLACK){
                RB_PARENT(pRbn)->uiColor = RB_BLACK;
                RB_GRAND(pRbn)->uiColor = RB_RED;
                __hoitRbRightRotate(pRbTree, RB_GRAND(pRbn));
            }
        }
        else {
            /* 父亲是右孩子, 叔叔节点为左侧, 且是红色 */
            if(RB_UNCLE_LEFT(pRbn)->uiColor == RB_RED){
                RB_PARENT(pRbn)->uiColor = RB_BLACK;
                RB_UNCLE_LEFT(pRbn)->uiColor = RB_BLACK;
                RB_GRAND(pRbn)->uiColor = RB_RED;
                pRbn = RB_GRAND(pRbn);
            }
            /* 该节点是右孩子 */
            else if(RB_IS_LEFT_CHILD(pRbn)){
                pRbn = RB_PARENT(pRbn);
                __hoitRbRightRotate(pRbTree, pRbn);
            }

            if(RB_IS_RIGHT_CHILD(pRbn) && RB_UNCLE_LEFT(pRbn)->uiColor == RB_BLACK){
                RB_PARENT(pRbn)->uiColor = RB_BLACK;
                RB_GRAND(pRbn)->uiColor = RB_RED;
                __hoitRbLeftRotate(pRbTree, RB_GRAND(pRbn));
            }
        }
    }
    pRbTree->pRbnRoot->uiColor = RB_BLACK;
}

VOID __hoitRbDeleteFixUp(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn){


}
/*********************************************************************************************************
** 函数名称: __hoitRbTraverse
** 功能描述: 中序遍历红黑树
** 输　入  : pRbTree          红黑树
**           pRbnRoot          树根
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __hoitRbTraverse(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbnRoot){
    if(pRbnRoot == pRbTree->pRbnGuard){
        return;
    }
    else
    {
        __hoitRbTraverse(pRbTree, pRbnRoot->pRbnLeft);
        printf("%d: %s left: %s right: %s\n", pRbnRoot->iKey, 
                                              pRbnRoot->uiColor == RB_RED ? "RED" : "BLACK", 
                                              RB_LEFT_CHILD(pRbnRoot)->uiColor == RB_RED ? "RED" : "BLACK", 
                                              RB_RIGHT_CHILD(pRbnRoot)->uiColor == RB_RED ? "RED" : "BLACK");
        __hoitRbTraverse(pRbTree, pRbnRoot->pRbnRight);   
    }
}


/*********************************************************************************************************
** 函数名称: hoitRbInsertNode
** 功能描述: 插入一个红黑树节点
** 输　入  : pRbTree          红黑树
**           pRbn               待插入节点              
** 输　出  : 成功返回True，失败返回False
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_RB_NODE hoitRbInsertNode(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn){
    PHOIT_RB_NODE       pRbnTrailing;
    PHOIT_RB_NODE       pRbnTraverse;

    pRbnTrailing = pRbTree->pRbnGuard;
    pRbnTraverse = pRbTree->pRbnRoot;
    
    while (pRbnTraverse != pRbTree->pRbnGuard)
    {
        pRbnTrailing = pRbnTraverse;
        if(pRbn->iKey < pRbnTraverse->iKey){
            pRbnTraverse = pRbnTraverse->pRbnLeft;
        }
        else pRbnTraverse = pRbnTraverse->pRbnRight;
    }

    pRbn->pRbnParent = pRbnTrailing;

    if(pRbnTrailing == pRbTree->pRbnGuard){
        pRbTree->pRbnRoot = pRbn;
    }
    else if(pRbn->iKey < pRbnTrailing->iKey){
        pRbnTrailing->pRbnLeft = pRbn;
    }
    else pRbnTrailing->pRbnRight = pRbn;

    pRbn->pRbnLeft = pRbTree->pRbnGuard;
    pRbn->pRbnRight = pRbTree->pRbnGuard;
    pRbn->uiColor = RB_RED;
    __hoitRbInsertFixUp(pRbTree, pRbn);

    return pRbn;
}


/*********************************************************************************************************
** 函数名称: hoitRbSearchNode
** 功能描述: 根据键值查找红黑树节点
** 输　入  : pRbTree          红黑树
**           iKey              键值              
** 输　出  : 成功返回节点指针，否则返回LW_NULL
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_RB_NODE hoitRbSearchNode(PHOIT_RB_TREE pRbTree, INT32 iKey){
    PHOIT_RB_NODE       pRbnTrailing;
    PHOIT_RB_NODE       pRbnTraverse;

    pRbnTrailing = pRbTree->pRbnGuard;
    pRbnTraverse = pRbTree->pRbnRoot;

    while (pRbnTraverse != pRbTree->pRbnGuard)
    {
        pRbnTrailing = pRbnTraverse;
        if(pRbnTraverse->iKey < iKey){
            pRbnTraverse = RB_RIGHT_CHILD(pRbnTraverse);
        }
        else pRbnTraverse = RB_RIGHT_CHILD(pRbnTraverse);
    }
    if(pRbnTraverse == pRbTree->pRbnGuard){
        return LW_NULL;
    }
    return pRbnTraverse;
}
//TODO: 删除、修改
BOOL hoitRbDeleteNode(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn){
    PHOIT_RB_NODE           pRbnTrailing;
    PHOIT_RB_NODE           pRbnChild;
    UINT32                  uiTrallingOriginColor;

    pRbnTrailing = pRbn;
    uiTrallingOriginColor = pRbnTrailing->uiColor;

    if(RB_RIGHT_CHILD(pRbn) == pRbTree->pRbnGuard){
        pRbnChild = RB_LEFT_CHILD(pRbn);
        __hoitRbTransplant(pRbTree, pRbn, pRbnChild);
    }
    else if (RB_LEFT_CHILD(pRbn) == pRbTree->pRbnGuard)
    {
        pRbnChild = RB_RIGHT_CHILD(pRbn);
        __hoitRbTransplant(pRbTree, pRbn, pRbnChild);
    }
    else
    {
        pRbnTrailing = __hoitRbMinimum(pRbTree, RB_RIGHT_CHILD(pRbn));        /* 寻找后继元素 */
        uiTrallingOriginColor = pRbnTrailing->uiColor;
        pRbnChild = RB_RIGHT_CHILD(pRbnTrailing);
        
        if(RB_PARENT(pRbnTrailing) == pRbn){
            pRbnChild->pRbnParent = pRbnTrailing;
        }
        else
        {
            __hoitRbTransplant(pRbTree, pRbnTrailing, pRbnChild);
            pRbnTrailing->pRbnRight = RB_RIGHT_CHILD(pRbn);
            pRbnTrailing->pRbnRight->pRbnParent = pRbnTrailing;
        }
        
        __hoitRbTransplant(pRbTree, pRbn, pRbnTrailing);
        pRbnTrailing->pRbnLeft = pRbn->pRbnLeft;
        pRbnTrailing->pRbnLeft->pRbnParent = pRbnTrailing;
        pRbnTrailing->uiColor = pRbn->uiColor;
    }

    if(uiTrallingOriginColor == RB_BLACK){
        __hoitRbDeleteFixUp(pRbTree, pRbnChild);
    }
}

/*********************************************************************************************************
** 函数名称: hoitInitRbTree
** 功能描述: 红黑树初始化
** 输　入  : NONE
** 输　出  : 成功返回True，失败返回False
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_RB_TREE hoitRbInitTree(){
    PHOIT_RB_TREE           pRbTree; 

    pRbTree = (PHOIT_RB_TREE)lib_malloc(sizeof(HOIT_RB_TREE));
    pRbTree->pRbnGuard = (PHOIT_RB_NODE)lib_malloc(sizeof(HOIT_RB_NODE));
    if(pRbTree < 0 || pRbTree->pRbnGuard < 0){
        printf("init red/black tree fail: memory low\n");
        return LW_NULL;
    }
    pRbTree->pRbnGuard->uiColor = RB_BLACK;
    pRbTree->pRbnRoot = pRbTree->pRbnGuard;
    return pRbTree;
}

#ifdef RB_TEST
VOID hoitRbTreeTest(){
    INT i;
    PHOIT_RB_TREE pRbTree;
    PHOIT_RB_NODE pRbn;
    INT testArray[10] = {8,11,14,15,1,2,4,5,7, 1};

    printf("\n [Red Black Tree Test Start] \n");
    pRbTree = hoitRbInitTree();
    
    for (i = 0; i < 10; i++)
    {
        pRbn = newHoitRbNode(testArray[i]);
        hoitRbInsertNode(pRbTree, pRbn);
    }

    __hoitRbTraverse(pRbTree, pRbTree->pRbnRoot);
    
}
#endif // DEBUG
