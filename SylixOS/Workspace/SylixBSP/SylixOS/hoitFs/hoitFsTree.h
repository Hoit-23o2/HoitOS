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
** 鏂�   浠�   鍚�: hoitFsTree.h
**
** 鍒�   寤�   浜�: Pan yanqi (娼樺欢楹�)
**
** 鏂囦欢鍒涘缓鏃ユ湡: 2021 骞� 03 鏈� 28 鏃�
**
** 鎻�        杩�: JFFS2-Like fragtree瀹炵幇
*********************************************************************************************************/

#ifndef SYLIXOS_EXTFS_HOITFS_HOITFSTREE_H_
#define SYLIXOS_EXTFS_HOITFS_HOITFSTREE_H_

#include "hoitType.h"
#include "hoitFsTreeUtil.h"
#include "hoitFsLib.h"

#define FT_TEST

/*********************************************************************************************************
  渚涙祴璇�
*********************************************************************************************************/
#ifdef FT_TEST
//typedef struct {
//    INT     Temp;
//}HOIT_FULL_DNODE;
//typedef HOIT_FULL_DNODE* PHOIT_FULL_DNODE;
#endif // DEBUG

/*********************************************************************************************************
  INODE_INFO鐨凢ragTree鐨勮妭鐐�
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
  INODE_INFO鎸囧悜鐨凢ragTree
*********************************************************************************************************/
typedef struct hoit_frag_tree
{
    PHOIT_RB_TREE pRbTree;
    UINT32 uiNCnt;                                  /* 鑺傜偣鏁扮洰 */
} HOIT_FRAG_TREE;
typedef HOIT_FRAG_TREE * PHOIT_FRAG_TREE;


/*********************************************************************************************************
  FragTree鍩烘湰鎿嶄綔
*********************************************************************************************************/
PHOIT_FRAG_TREE            hoitInitFragTree(VOID);                                                              /* 鍒濆鍖栨爲 */
PHOIT_FRAG_TREE_NODE       hoitFragTreeInsertNode(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn);                       /* 鎻掑叆涓�涓妭鐐� */
PHOIT_FRAG_TREE_NODE       hoitFragTreeSearchNode(PHOIT_FRAG_TREE pFTTree, INT32 iKey);                                      /* 鏌ユ壘涓�涓妭鐐� */
BOOL                       hoitFragTreeDeleteNode(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn);                       /* 鍒犻櫎涓�涓妭鐐� */
VOID                       hoitFragTreeTraverse(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTnRoot);                         /* 涓簭閬嶅巻FragTree */

/*********************************************************************************************************
  FragTree杩涢樁鎿嶄綔 - 涓庝笅灞傚疄浣撴搷浣�
*********************************************************************************************************/
//TODO锛氳鍙朏ragTree锛岀劧鍚庡悜涓嬭鍙栨暟鎹疄浣擄紝鍩烘湰閫昏緫涓哄厛璇籆ache锛孋ache鏈懡涓啀璇籪lash
error_t hoitFragTreeRead(PHOIT_FRAG_TREE pFTTree, UINT uiOfs, UINT32 uiSize, PCHAR pContent);
//TODO锛氭悳绱ragTree锛屽鎵綩verlay - 4绉嶆儏鍐碉紝鐒跺悗鍋氬嚭鐩稿簲淇敼锛� 閬垮厤鎺夌數锛屾瘡娆℃瀯寤烘爲鍚庨兘璋冪敤涓�娆ixUp
error_t hoitFragTreeOverlayFixUp(PHOIT_FRAG_TREE pFTTree);
//TODO锛氬啓鍏ragTree锛岀劧鍚庤皟鐢ㄦ悳绱ragTree杩涜閲嶅彔妫�娴嬩笌淇敼锛岀劧鍚庡啓鍏ache锛孋ache鏈懡涓紝鍏堣鍑哄啀鍐欏叆锛屽懡涓洿鎺ュ啓鍏ワ紝鍐欐弧鍚嶧lush杩沠lash锛岃繖閲屽啓鍏ache鐨勬椂鍊欏彲浠ュ仛涓�浜涘浠斤紝渚嬪Trascation鎴栬�匧og
error_t hoitFragTreeWrite(PHOIT_FRAG_TREE pFTTree, UINT32 uiOfs, UINT32 uiSize, PCHAR pContent);
//(TODO)锛氭悳闆咶ragTree  iKey澶т簬绛変簬鏌愪釜鍊肩殑鎵�鏈夎妭鐐�

#ifdef FT_TEST
VOID hoitFTTreeTest();
#endif // DEBUG


#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSTREE_H_ */
