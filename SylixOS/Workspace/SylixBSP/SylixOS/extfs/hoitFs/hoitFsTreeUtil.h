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


static inline PHOIT_RB_NODE newHoitRbNode(INT32 iKey){
    PHOIT_RB_NODE pRbn = (PHOIT_RB_NODE)lib_malloc(sizeof(HOIT_RB_NODE));
    pRbn->iKey = iKey;
    return pRbn;
}

PHOIT_RB_TREE     hoitRbInitTree(VOID);                                          /* ÂàùÂ?ÂåñRBÊ†? */
PHOIT_RB_NODE     hoitRbInsertNode(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn);   /* ÊèíÂÖ•‰∏?‰∏?äÇÁÇ? */
PHOIT_RB_NODE     hoitRbSearchNode(PHOIT_RB_TREE pRbTree, INT32 iKey);           /* Êü•Êâæ‰∏?‰∏?äÇÁÇ? */
BOOL              hoitRbDeleteNode(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn);   /* Âà†Èô§‰∏?‰∏?äÇÁÇ? */

#ifdef RB_TEST
VOID hoitRbTreeTest();
#endif // DEBUG

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSTREEUTIL_H_ */
