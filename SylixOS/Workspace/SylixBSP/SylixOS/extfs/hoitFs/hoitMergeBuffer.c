/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: HoitMergeBuffer.c
**
** ��   ��   ��: Hu Zhisheng
**
** �ļ���������: 2021 �� 07 �� 10 ��
**
** ��        ��: Hoit�ļ�ϵͳΪÿ���ļ�������MergeBuffer�ĺ�����, ���ںϲ�д��С����
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL

#include "hoitMergeBuffer.h"
#include "hoitFsLib.h"
#include "hoitFsTree.h"

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0

#ifndef HOITFSLIB_DISABLE

/*********************************************************************************************************
** ��������: __hoit_new_merge_buffer
** ��������: Ϊһ���ļ��½�MergeBuffer
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_new_merge_buffer(PHOIT_INODE_INFO pInodeInfo) {
    if (pInodeInfo->HOITN_pMergeBuffer == LW_NULL) {
        pInodeInfo->HOITN_pMergeBuffer              = (PHOIT_MERGE_BUFFER)lib_malloc(sizeof(HOIT_MERGE_BUFFER));
        pInodeInfo->HOITN_pMergeBuffer->pList       = LW_NULL;
        pInodeInfo->HOITN_pMergeBuffer->threshold   = HOIT_MERGE_BUFFER_THRESHOLD;
        pInodeInfo->HOITN_pMergeBuffer->size        = 0;
    }
    return LW_TRUE;
}
/*********************************************************************************************************
** ��������: __hoit_new_merge_entry
** ��������: Ϊһ���ļ���һ��������ڵ��½�һ��WriteEntry
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_new_merge_entry(PHOIT_INODE_INFO pInodeInfo, PHOIT_MERGE_BUFFER pMergeBuffer, PHOIT_FRAG_TREE_NODE pTreeNode) {
    /* û����ص�Entry, �½�һ��Entry */
    PHOIT_MERGE_ENTRY pMergeEntry = (PHOIT_MERGE_ENTRY)lib_malloc(sizeof(HOIT_MERGE_ENTRY));
    pMergeEntry->pTreeNode = pTreeNode;
    pMergeEntry->pNext = LW_NULL;
    pMergeEntry->pPrev = LW_NULL;
    pTreeNode->pMergeEntry = pMergeEntry;

    if (pMergeBuffer->pList == LW_NULL) {
        pMergeBuffer->pList = pMergeEntry;
        pMergeBuffer->size += 1;
    }
    else {
        pMergeEntry->pNext = pMergeBuffer->pList;
        pMergeBuffer->pList->pPrev = pMergeEntry;
        pMergeBuffer->pList = pMergeEntry;
        pMergeBuffer->size += 1;
    }
    if (pMergeBuffer->size >= pMergeBuffer->threshold) {    /* ��MergeBuffer�����ӵ����ݽڵ���Ŀ������ֵʱ���кϲ� */
        __hoit_refresh_merge_buffer(pInodeInfo);
    }

    return LW_TRUE;
}

/*********************************************************************************************************
** ��������: __hoit_del_merge_entry
** ��������: 
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_del_merge_entry(PHOIT_MERGE_BUFFER pMergeBuffer, PHOIT_MERGE_ENTRY pMergeEntry) {
    if (pMergeEntry == LW_NULL) {
        return LW_FALSE;
    }
    if (pMergeEntry->pPrev) {
        pMergeEntry->pPrev->pNext = pMergeEntry->pNext;
    }
    if (pMergeEntry->pNext) {
        pMergeEntry->pNext->pPrev = pMergeEntry->pPrev;
    }

    if (pMergeBuffer->pList == pMergeEntry) {
        pMergeBuffer->pList = pMergeEntry->pNext;
    }

    __SHEAP_FREE(pMergeEntry);

    return LW_TRUE;
}

/*********************************************************************************************************
** ��������: __hoit_free_merge_buffer
** ��������:
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_free_merge_buffer(PHOIT_INODE_INFO pInodeInfo) {
    if (pInodeInfo == LW_NULL || pInodeInfo->HOITN_pMergeBuffer == LW_NULL) {
        return LW_FALSE;
    }

    PHOIT_MERGE_BUFFER pMergeBuffer     = pInodeInfo->HOITN_pMergeBuffer;
    PHOIT_MERGE_ENTRY pNowWriteEntry    = pMergeBuffer->pList;
    PHOIT_MERGE_ENTRY pNextWriteEntry   = LW_NULL;
    while (pNowWriteEntry) {
        pNextWriteEntry = pNowWriteEntry->pNext;
        __SHEAP_FREE(pNowWriteEntry);
        pNowWriteEntry = pNextWriteEntry;
    }
    __SHEAP_FREE(pMergeBuffer);
    pInodeInfo->HOITN_pMergeBuffer = LW_NULL;

    return LW_TRUE;
}

/*********************************************************************************************************
** ��������: __hoit_refresh_merge_buffer
** ��������: ��MergeBuffer�е����е����е����ݽ������ںϲ�
** �䡡��  : 
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_refresh_merge_buffer(PHOIT_INODE_INFO pInodeInfo) {
    PHOIT_MERGE_BUFFER pMergeBuffer = pInodeInfo->HOITN_pMergeBuffer;
    INT32   i;
    if (pMergeBuffer == LW_NULL) {
        return LW_FALSE;
    }
    /* ������ */
    for (i = 0; i < pMergeBuffer->size-1; i++) {
        PHOIT_MERGE_ENTRY pNowEntry = pMergeBuffer->pList;
        PHOIT_MERGE_ENTRY pNextEntry = LW_NULL;
        while(pNowEntry)
        {
            pNextEntry = pNowEntry->pNext;
            if (pNextEntry) {
                if (pNowEntry->pTreeNode->uiOfs > pNextEntry->pTreeNode->uiOfs) {
                    PHOIT_FRAG_TREE_NODE pTempNode = pNowEntry->pTreeNode;
                    pNowEntry->pTreeNode = pNextEntry->pTreeNode;
                    pNextEntry->pTreeNode = pTempNode;
                }
            }
            pNowEntry = pNextEntry;
        }
    }

    /* TODO */
    PHOIT_MERGE_ENTRY pNowEntry     = pMergeBuffer->pList;
    PHOIT_MERGE_ENTRY pLastEntry    = LW_NULL;
    UINT32 left     = -1;
    UINT32 right    = -1;
    UINT32 count    = 0;
    for (; pNowEntry; pNowEntry = pNowEntry->pNext)
    {
        pLastEntry = pNowEntry; /* pLastEntry�����¼���һ��Entry */

        if (left == -1) {
            left    = pNowEntry->pTreeNode->uiOfs;
            right   = left + pNowEntry->pTreeNode->uiSize;
            count   = 1;
        }
        else {
            if (right == pNowEntry->pTreeNode->uiOfs) {
                right += pNowEntry->pTreeNode->uiSize;
                count += 1;
            }
            else {
                if (count > 1) {    /* �ж���ڵ���Խ��кϲ� */
                    int jump_count = count;
                    while (jump_count) {
                        pLastEntry = pLastEntry->pPrev;
                        jump_count -= 1;
                    }
                    char* pvBuffer = (char*)lib_malloc(right - left);
                    hoitFragTreeRead(pInodeInfo->HOITN_rbtree, left, right-left, pvBuffer);
                    __hoit_write(pInodeInfo, pvBuffer, right - left, left, 0);
                }
                left = pNowEntry->pTreeNode->uiOfs;
                right = left + pNowEntry->pTreeNode->uiSize;
                count = 1;
            }
        }
    }
    if (count > 1) {    /* �ж���ڵ���Խ��кϲ� */
        int jump_count = count;
        while (jump_count) {
            pLastEntry = pLastEntry->pPrev;
            jump_count -= 1;
        }
        char* pvBuffer = (char*)lib_malloc(right - left);
        hoitFragTreeRead(pInodeInfo->HOITN_rbtree, left, right - left, pvBuffer);
        __hoit_write(pInodeInfo, pvBuffer, right - left, left, 0);
    }
    return LW_TRUE;
}


#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
#endif //HOITFSLIB_DISABLE
