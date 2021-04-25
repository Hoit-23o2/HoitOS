

#ifndef SYLIXOS_EXTFS_HOITFS_HOITFSTREE_H_
#define SYLIXOS_EXTFS_HOITFS_HOITFSTREE_H_

#include "hoitType.h"
#include "hoitFsTreeUtil.h"
#include "hoitFsLib.h"

//#define FT_TEST
#define FT_DEBUG

/*********************************************************************************************************
  �ϲ�ӿ� - ������
*********************************************************************************************************/
#ifdef FT_TEST
typedef struct hoit_raw_header
{
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;
    mode_t              file_type;
    UINT                ino;
} HOIT_RAW_HEADER;
typedef HOIT_RAW_HEADER * PHOIT_RAW_HEADER;

typedef struct hoit_raw_info {
    struct hoit_raw_info *     next_phys;
    UINT                       phys_addr;
    UINT                       totlen;
} HOIT_RAW_INFO;
typedef HOIT_RAW_INFO * PHOIT_RAW_INFO;

typedef struct hoit_full_dnode{
    struct hoit_full_dnode *  HOITFD_next;
    PHOIT_RAW_INFO            HOITFD_raw_info;
    UINT                      HOITFD_offset;                                  /* ���ļ����ƫ���� */
    UINT                      HOITFD_length;                                  /* ��Ч�����ݳ��� */
    UINT                      HOITFD_version;                                 /* �汾�� */
} HOIT_FULL_DNODE;
typedef HOIT_FULL_DNODE* PHOIT_FULL_DNODE;

//typedef struct hoit_volume{
//
//} HOIT_VOLUME;
//typedef HOIT_VOLUME * PHOIT_VOLUME;

PHOIT_FULL_DNODE __hoit_truncate_full_dnode(PHOIT_VOLUME pfs, PHOIT_FULL_DNODE pFDnode, UINT uiOffset, UINT uiSize);
BOOL             __hoit_delete_full_dnode(PHOIT_VOLUME pfs, PHOIT_FULL_DNODE pFDnode, BOOL bDoDelete);

#endif // FT_TEST

static inline PHOIT_FRAG_TREE_NODE newHoitFragTreeNode(PHOIT_FULL_DNODE pFDnode, UINT32 uiSize, UINT32 uiOfs, UINT32 iKey){
    PHOIT_FRAG_TREE_NODE pFTn = (PHOIT_FRAG_TREE_NODE)lib_malloc(sizeof(HOIT_FRAG_TREE_NODE));
    pFTn->pRbn.iKey = iKey;
    pFTn->uiOfs = uiOfs;
    pFTn->uiSize = uiSize;
    pFTn->pFDnode = pFDnode;
    return pFTn;
}


/*********************************************************************************************************
 PHOIT_FRAG_TREE_LIST_NODE���캯��
*********************************************************************************************************/
static inline PHOIT_FRAG_TREE_LIST_NODE newFragTreeListNode(PHOIT_FRAG_TREE_NODE pFTn){
   PHOIT_FRAG_TREE_LIST_NODE pFTlistNode;
   pFTlistNode = (PHOIT_FRAG_TREE_LIST_NODE)lib_malloc(sizeof(HOIT_FRAG_TREE_LIST_NODE));
   pFTlistNode->pFTn = pFTn;
   pFTlistNode->pFTlistNext = LW_NULL;
   return pFTlistNode;
}

/*********************************************************************************************************
 ��ʼ���ڵ�����
*********************************************************************************************************/
static inline PHOIT_FRAG_TREE_LIST_HEADER hoitFragTreeListInit(VOID){
    PHOIT_FRAG_TREE_LIST_HEADER pFTlistHeader;
    pFTlistHeader = (PHOIT_FRAG_TREE_LIST_HEADER)lib_malloc(sizeof(HOIT_FRAG_TREE_LIST_HEADER));
    pFTlistHeader->uiHighBound = INT_MAX;
    pFTlistHeader->uiLowBound = INT_MIN;
    pFTlistHeader->pFTlistHeader = newFragTreeListNode(LW_NULL);
    pFTlistHeader->uiNCnt = 0;
    return pFTlistHeader;
}
/*********************************************************************************************************
 �������в���ڵ�
*********************************************************************************************************/
static inline VOID hoitFragTreeListInsertNode(PHOIT_FRAG_TREE_LIST_HEADER pFTlistHeader, PHOIT_FRAG_TREE_LIST_NODE pTFlistNode){
    PHOIT_FRAG_TREE_LIST_NODE pFTlistTrailling;
    PHOIT_FRAG_TREE_LIST_NODE pFTlistTraverse = pFTlistHeader->pFTlistHeader;
    while (pFTlistTraverse != LW_NULL)
    {
        pFTlistTrailling = pFTlistTraverse;
        pFTlistTraverse = pFTlistTraverse->pFTlistNext;
    }
    pFTlistTrailling->pFTlistNext = pTFlistNode;
}
/*********************************************************************************************************
 ɾ��������ĳ�ڵ㣬���ͷ��ڴ�
*********************************************************************************************************/
static inline VOID hoitFragTreeListDeleteNode(PHOIT_FRAG_TREE_LIST_HEADER pFTlistHeader, PHOIT_FRAG_TREE_LIST_NODE pTFlistNode){
    PHOIT_FRAG_TREE_LIST_NODE pFTlistTrailling;
    PHOIT_FRAG_TREE_LIST_NODE pFTlistTraverse;

    pFTlistTraverse = pFTlistHeader->pFTlistHeader;
    pFTlistTrailling = pFTlistTraverse;

    while (pFTlistTraverse != LW_NULL)
    {
        if(pFTlistTraverse == pTFlistNode){
            pFTlistTrailling->pFTlistNext = pFTlistTraverse->pFTlistNext;
            pFTlistTraverse->pFTlistNext = LW_NULL;
            break;
        }
        pFTlistTrailling = pFTlistTraverse;
        pFTlistTraverse = pFTlistTraverse->pFTlistNext;
    }
}
/*********************************************************************************************************
 ɾ������
*********************************************************************************************************/
static inline VOID hoitFragTreeListFree(PHOIT_FRAG_TREE_LIST_HEADER pFTlistHeader){
    PHOIT_FRAG_TREE_LIST_NODE pFTlistTraverse;
    PHOIT_FRAG_TREE_LIST_NODE pFTlistNext;

    pFTlistTraverse = pFTlistHeader->pFTlistHeader;
    while (pFTlistTraverse != LW_NULL)
    {
        pFTlistNext = pFTlistTraverse->pFTlistNext;
        lib_free(pFTlistTraverse);
        pFTlistTraverse = pFTlistNext;
    }
    lib_free(pFTlistHeader);
}


/*********************************************************************************************************
  FragTree�������� - �̲߳���ȫ - ��ͬ����ͬʱ���ļ�����Ҫ��������
*********************************************************************************************************/
PHOIT_FRAG_TREE               hoitInitFragTree(PHOIT_VOLUME pfs);                                                                           /* ��ʼ���� */
PHOIT_FRAG_TREE_NODE          hoitFragTreeInsertNode(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn);                       /* ����һ���ڵ� */
PHOIT_FRAG_TREE_NODE          hoitFragTreeSearchNode(PHOIT_FRAG_TREE pFTTree, INT32 iKey);                                      /* ����һ���ڵ� */
BOOL                          hoitFragTreeDeleteNode(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn, BOOL bDoDelete);                       /* ɾ��һ���ڵ� */
BOOL                          hoitFragTreeDeleteRange(PHOIT_FRAG_TREE pFTTree, INT32 iKeyLow, INT32 iKeyHigh, BOOL bDoDelete);
BOOL                          hoitFragTreeDeleteTree(PHOIT_FRAG_TREE pFTTree, BOOL bDoDelete);
VOID                          hoitFragTreeTraverse(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTnRoot);                     /* �������FragTree */
PHOIT_FRAG_TREE_LIST_HEADER   hoitFragTreeCollectRange(PHOIT_FRAG_TREE pFTTree, INT32 iKeyLow, INT32 iKeyHigh);

/*********************************************************************************************************
  FragTree
*********************************************************************************************************/
//TODO����ȡFragTree��Ȼ�����¶�ȡ����ʵ�壬�����߼�Ϊ�ȶ�Cache��Cacheδ�����ٶ�flash
//!�ò��ֿ����Ƴ�
BOOL hoitFragTreeRead(PHOIT_FRAG_TREE pFTTree, UINT32 uiOfs, UINT32 uiSize, PCHAR pContent);


//TODO������FragTree��Ѱ��Overlay - 4�������Ȼ��������Ӧ�޸ģ� ������磬ÿ�ι������󶼵���һ��FixUp
BOOL hoitFragTreeOverlayFixUp(PHOIT_FRAG_TREE pFTTree);

//!�ò����Ƴ�
////д��FragTree��Ȼ���������FragTree�����ص�������޸ģ�Ȼ��д��Cache��Cacheδ���У��ȶ�����д�룬����ֱ��д�룬д����Flush��flash������д��Cache��ʱ�������һЩ���ݣ�����Trascation����Log
//// BOOL hoitFragTreeWrite(PHOIT_FRAG_TREE pFTTree, UINT32 uiOfs, UINT32 uiSize, PCHAR pContent);

#ifdef FT_TEST
VOID hoitFTTreeTest();
#endif // DEBUG


#endif /* SYLIXOS_EXTFS_HOITFS_HOITFSTREE_H_ */
