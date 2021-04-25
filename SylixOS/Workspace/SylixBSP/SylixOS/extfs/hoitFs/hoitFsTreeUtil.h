/*********************************************************************************************************
**
**                                    �?���?���?源组�?
**
**                                   嵌入式实时操作系�?
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** �?   �?   �?: hoitFsTreeUtil.h
**
** �?   �?   �?: Pan yanqi (潘延�?)
**
** 文件创建日期: 2021 �? 03 �? 28 �?
**
** �?        �?: JFFS2-Like 红黑树数�?���?
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#ifndef SYLIXOS_EXTFS_HOITFS_HOITFSTREEUTIL_H_
#define SYLIXOS_EXTFS_HOITFS_HOITFSTREEUTIL_H_

#define RB_TEST

#include "hoitType.h"

#define RB_BLACK    1
#define RB_RED      0
/*********************************************************************************************************
  红黑树节点定�?
*********************************************************************************************************/
typedef struct hoit_rb_node
{
    UINT32 uiColor;
    INT32  iKey;
    struct hoit_rb_node* pRbnLeft;
    struct hoit_rb_node* pRbnRight;   
    struct hoit_rb_node* pRbnParent;
} HOIT_RB_NODE;

typedef HOIT_RB_NODE * PHOIT_RB_NODE;

static inline PHOIT_RB_NODE newHoitRbNode(INT32 iKey){
    PHOIT_RB_NODE pRbn = (PHOIT_RB_NODE)lib_malloc(sizeof(HOIT_RB_NODE));
    pRbn->iKey = iKey;
    return pRbn;
}
/*********************************************************************************************************
  红黑树定�?
*********************************************************************************************************/
typedef struct hoit_rb_tree
{
    PHOIT_RB_NODE pRbnGuard;            /* 哨兵 */
    PHOIT_RB_NODE pRbnRoot;             /* 根节�? */
} HOIT_RB_TREE;

typedef HOIT_RB_TREE * PHOIT_RB_TREE;



PHOIT_RB_TREE     hoitRbInitTree(VOID);                                          /* 初�?化RB�? */
PHOIT_RB_NODE     hoitRbInsertNode(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn);   /* 插入�?�?���? */
PHOIT_RB_NODE     hoitRbSearchNode(PHOIT_RB_TREE pRbTree, INT32 iKey);           /* 查找�?�?���? */
BOOL              hoitRbDeleteNode(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn);   /* 删除�?�?���? */

#ifdef RB_TEST
VOID hoitRbTreeTest();
#endif // DEBUG

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSTREEUTIL_H_ */
