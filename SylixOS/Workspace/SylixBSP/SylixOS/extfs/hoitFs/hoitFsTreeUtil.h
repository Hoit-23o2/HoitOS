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


static inline PHOIT_RB_NODE newHoitRbNode(INT32 iKey){
    PHOIT_RB_NODE pRbn = (PHOIT_RB_NODE)lib_malloc(sizeof(HOIT_RB_NODE));
    pRbn->iKey = iKey;
    return pRbn;
}

PHOIT_RB_TREE     hoitRbInitTree(VOID);                                          /* 初�?化RB�? */
PHOIT_RB_NODE     hoitRbInsertNode(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn);   /* 插入�?�?���? */
PHOIT_RB_NODE     hoitRbSearchNode(PHOIT_RB_TREE pRbTree, INT32 iKey);           /* 查找�?�?���? */
BOOL              hoitRbDeleteNode(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn);   /* 删除�?�?���? */

#ifdef RB_TEST
VOID hoitRbTreeTest();
#endif // DEBUG

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSTREEUTIL_H_ */
