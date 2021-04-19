/*********************************************************************************************************
**
**                                    涓浗杞欢寮�婧愮粍缁�
**
**                                   宓屽叆寮忓疄鏃舵搷浣滅郴缁�
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------鏂囦欢淇℃伅--------------------------------------------------------------------------------
**
** 鏂�   浠�   鍚�: hoitFsTreeUtil.h
**
** 鍒�   寤�   浜�: Pan yanqi (娼樺欢楹�)
**
** 鏂囦欢鍒涘缓鏃ユ湡: 2021 骞� 03 鏈� 28 鏃�
**
** 鎻�        杩�: JFFS2-Like 绾㈤粦鏍戞暟鎹粨鏋�
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
  绾㈤粦鏍戣妭鐐瑰畾涔�
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
  绾㈤粦鏍戝畾涔�
*********************************************************************************************************/
typedef struct hoit_rb_tree
{
    PHOIT_RB_NODE pRbnGuard;            /* 鍝ㄥ叺 */
    PHOIT_RB_NODE pRbnRoot;             /* 鏍硅妭鐐� */
} HOIT_RB_TREE;

typedef HOIT_RB_TREE * PHOIT_RB_TREE;



PHOIT_RB_TREE     hoitRbInitTree(VOID);                                          /* 鍒濆鍖朢B鏍� */
PHOIT_RB_NODE     hoitRbInsertNode(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn);   /* 鎻掑叆涓�涓妭鐐� */
PHOIT_RB_NODE     hoitRbSearchNode(PHOIT_RB_TREE pRbTree, INT32 iKey);           /* 鏌ユ壘涓�涓妭鐐� */
BOOL              hoitRbDeleteNode(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn);   /* 鍒犻櫎涓�涓妭鐐� */

#ifdef RB_TEST
VOID hoitRbTreeTest();
#endif // DEBUG

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSTREEUTIL_H_ */
