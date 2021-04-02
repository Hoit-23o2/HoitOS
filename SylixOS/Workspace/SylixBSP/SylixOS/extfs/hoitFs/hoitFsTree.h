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
#include "hoitFsLib.h"

#define FT_TEST

/*********************************************************************************************************
  供测试
*********************************************************************************************************/
#ifdef FT_TEST
typedef struct {

}HOIT_FULL_DNODE;
typedef HOIT_FULL_DNODE* PHOIT_FULL_DNODE;
#endif // DEBUG

/*********************************************************************************************************
  INODE_INFO的FragTree的节点
*********************************************************************************************************/
typedef struct hoit_frag_tree_node
{
    HOIT_RB_NODE pRbn;
    // struct jffs2_full_dnode *node; 
    PHOIT_FULL_DNODE pFDnode;
	  UINT32 uiSize;
	  UINT32 uiOfs; 
} HOIT_FRAG_TREE_NODE;

typedef HOIT_FRAG_TREE_NODE *PHOIT_FRAG_TREE_NODE;

static inline PHOIT_FRAG_TREE_NODE newHoitFragTreeNode(PHOIT_FULL_DNODE pFDnode, UINT32 uiSize, UINT32 uiOfs, UINT32 iKey){
    PHOIT_FRAG_TREE_NODE pFTn = (PHOIT_FRAG_TREE_NODE)lib_malloc(sizeof(HOIT_FRAG_TREE_NODE));
    pFTn->pRbn.iKey = iKey;
    pFTn->uiOfs = uiOfs;
    pFTn->uiSize = uiSize;
    pFTn->pFDnode = pFDnode;
    return pFTn;
}
/*********************************************************************************************************
  INODE_INFO指向的FragTree
*********************************************************************************************************/
typedef struct hoit_frag_tree
{
    PHOIT_RB_TREE pRbTree;
    UINT32 uiNCnt;                                  /* 节点数目 */
} HOIT_FRAG_TREE;
typedef HOIT_FRAG_TREE * PHOIT_FRAG_TREE;


/*********************************************************************************************************
  FragTree基本操作
*********************************************************************************************************/
PHOIT_FRAG_TREE            hoitInitFragTree(VOID);                                                              /* 初始化树 */
PHOIT_FRAG_TREE_NODE       hoitFragTreeInsertNode(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn);                       /* 插入一个节点 */
PHOIT_FRAG_TREE_NODE       hoitFragTreeSearchNode(PHOIT_FRAG_TREE pFTTree, INT32 iKey);                                      /* 查找一个节点 */
BOOL                       hoitFragTreeDeleteNode(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn);                       /* 删除一个节点 */
VOID                       hoitFragTreeTraverse(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTnRoot);                         /* 中序遍历FragTree */

/*********************************************************************************************************
  FragTree进阶操作 - 与下层实体操作
*********************************************************************************************************/
//TODO：读取FragTree，然后向下读取数据实体，基本逻辑为先读Cache，Cache未命中再读flash
error_t hoitFragTreeRead(PHOIT_FRAG_TREE pFTTree, UINT uiOfs, UINT32 uiSize, PCHAR pContent);
//TODO：搜索FragTree，寻找Overlay - 4种情况，然后做出相应修改， 避免掉电，每次构建树后都调用一次FixUp
error_t hoitFragTreeOverlayFixUp(PHOIT_FRAG_TREE pFTTree);
//TODO：写入FragTree，然后调用搜索FragTree进行重叠检测与修改，然后写入Cache，Cache未命中，先读出再写入，命中直接写入，写满后Flush进flash，这里写入Cache的时候可以做一些备份，例如Trascation或者Log
error_t hoitFragTreeWrite(PHOIT_FRAG_TREE pFTTree, UINT32 uiOfs, UINT32 uiSize, PCHAR pContent);
//(TODO)：搜集FragTree  iKey大于等于某个值的所有节点

#ifdef FT_TEST
VOID hoitFTTreeTest();
#endif // DEBUG


#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSTREE_H_ */
