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
** ��   ��   ��: hoitFsTreeUtil.h
**
** ��   ��   ��: Pan yanqi (������)
**
** �ļ���������: 2021 �� 03 �� 28 ��
**
** ��        ��: JFFS2-Like ��������ݽṹ
*********************************************************************************************************/

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#ifndef SYLIXOS_EXTFS_HOITFS_HOITFSTREEUTIL_H_
#define SYLIXOS_EXTFS_HOITFS_HOITFSTREEUTIL_H_


#include "hoitType.h"

#define RB_BLACK    1
#define RB_RED      0


static inline PHOIT_RB_NODE newHoitRbNode(INT32 iKey){
    PHOIT_RB_NODE pRbn = (PHOIT_RB_NODE)lib_malloc(sizeof(HOIT_RB_NODE));
    pRbn->iKey = iKey;
    return pRbn;
}

PHOIT_RB_TREE     hoitRbInitTree(VOID);                             /* ��ʼ��RB�� */
PHOIT_RB_NODE     hoitRbInsertNode(PHOIT_RB_TREE, PHOIT_RB_NODE);   /* ����һ���ڵ� */
PHOIT_RB_NODE     hoitRbSearchNode(PHOIT_RB_TREE, INT32);           /* ����һ���ڵ� */
BOOL              hoitRbDeleteNode(PHOIT_RB_TREE, PHOIT_RB_NODE);   /* ɾ��һ���ڵ� */

#ifdef RB_TEST
VOID hoitRbTreeTest();
#endif // DEBUG

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSTREEUTIL_H_ */
