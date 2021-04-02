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

#ifndef SYLIXOS_EXTFS_HOITFS_HOITFSTREE_H_
#define SYLIXOS_EXTFS_HOITFS_HOITFSTREE_H_

#include "hoitType.h"
#include "hoitFsTreeUtil.h"
/*********************************************************************************************************
  hoit
*********************************************************************************************************/
typedef struct hoit_inode_tree_node
{
    PHOIT_RB_NODE pRbn;
    // struct jffs2_full_dnode *node; 
	  UINT32 uiSize;
	  UINT32 uiOfs; /* The offset to which this fragment belongs */
} HOIT_INODE_TREE_NODE;

typedef HOIT_INODE_TREE_NODE *PHOIT_INODE_TREE_NODE;
 

#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSTREE_H_ */
