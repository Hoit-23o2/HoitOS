/*********************************************************************************************************
**
**                                    ‰∏?õΩËΩ?ª∂Âº?Ê∫êÁªÑÁª?
**
**                                   ÂµåÂÖ•ÂºèÂÆûÊó∂Êìç‰ΩúÁ≥ªÁª?
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------Êñá‰ª∂‰ø°ÊÅØ--------------------------------------------------------------------------------
**
** Êñ?   ‰ª?   Âê?: hoitFsTreeUtil.h
**
** Âà?   Âª?   ‰∫?: Pan yanqi (ÊΩòÂª∂È∫?)
**
** Êñá‰ª∂ÂàõÂª∫Êó•Êúü: 2021 Âπ? 03 Êú? 28 Êó?
**
** Êè?        Ëø?: JFFS2-Like Á∫¢ÈªëÊ†ëÊï∞Êç?ªìÊû?
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
  Á∫¢ÈªëÊ†ëËäÇÁÇπÂÆö‰π?
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
  Á∫¢ÈªëÊ†ëÂÆö‰π?
*********************************************************************************************************/
typedef struct hoit_rb_tree
{
    PHOIT_RB_NODE pRbnGuard;            /* Âì®ÂÖµ */
    PHOIT_RB_NODE pRbnRoot;             /* Ê†πËäÇÁÇ? */
} HOIT_RB_TREE;

typedef HOIT_RB_TREE * PHOIT_RB_TREE;



PHOIT_RB_TREE     hoitRbInitTree(VOID);                                          /* ÂàùÂ?ÂåñRBÊ†? */
PHOIT_RB_NODE     hoitRbInsertNode(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn);   /* ÊèíÂÖ•‰∏?‰∏?äÇÁÇ? */
PHOIT_RB_NODE     hoitRbSearchNode(PHOIT_RB_TREE pRbTree, INT32 iKey);           /* Êü•Êâæ‰∏?‰∏?äÇÁÇ? */
BOOL              hoitRbDeleteNode(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn);   /* Âà†Èô§‰∏?‰∏?äÇÁÇ? */

#ifdef RB_TEST
VOID hoitRbTreeTest();
#endif // DEBUG

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSTREEUTIL_H_ */
