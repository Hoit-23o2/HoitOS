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
** ��   ��   ��: HoitWriteBuffer.c
**
** ��   ��   ��: Hu Zhisheng
**
** �ļ���������: 2021 �� 07 �� 10 ��
**
** ��        ��: Hoit�ļ�ϵͳΪÿ���ļ�������WriteBuffer�ĺ�����, ���ںϲ�д��С����
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL

#include "hoitWriteBuffer.h"
#include "hoitFsLib.h"
#include "hoitFsTree.h"

/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0

#ifndef HOITFSLIB_DISABLE

/*********************************************************************************************************
** ��������: __hoit_new_write_buffer
** ��������: Ϊһ���ļ��½�WriteBuffer
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_new_write_buffer(PHOIT_INODE_INFO pInodeInfo) {
    if (pInodeInfo->HOITN_pWriteBuffer == LW_NULL) {
        pInodeInfo->HOITN_pWriteBuffer->pList       = LW_NULL;
        pInodeInfo->HOITN_pWriteBuffer->threshold   = HOIT_WRITE_BUFFER_THRESHOLD;
        pInodeInfo->HOITN_pWriteBuffer->size        = 0;
    }
    return LW_TRUE;
}
/*********************************************************************************************************
** ��������: __hoit_new_write_entry
** ��������: Ϊһ���ļ���һ��������ڵ��½�һ��WriteEntry
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_new_write_entry(PHOIT_INODE_INFO pInodeInfo, PHOIT_WRITE_BUFFER pWriteBuffer, PHOIT_FRAG_TREE_NODE pTreeNode) {
    /* û����ص�Entry, �½�һ��Entry */
    PHOIT_WRITE_ENTRY pWriteEntry = (PHOIT_WRITE_ENTRY)lib_malloc(sizeof(HOIT_WRITE_ENTRY));
    pWriteEntry->pTreeNode = pTreeNode;
    pWriteEntry->pNext = LW_NULL;
    pWriteEntry->pPrev = LW_NULL;
    pTreeNode->pWriteEntry = pWriteEntry;

    if (pWriteBuffer->pList == LW_NULL) {
        pWriteBuffer->pList = pWriteEntry;
        pWriteBuffer->size += 1;
    }
    else {
        pWriteEntry->pNext = pWriteBuffer->pList;
        pWriteBuffer->pList->pPrev = pWriteEntry;
        pWriteBuffer->pList = pWriteEntry;
        pWriteBuffer->size += 1;
    }
    if (pWriteBuffer->size >= pWriteBuffer->threshold) {    /* ��WriteBuffer�����ӵ����ݽڵ���Ŀ������ֵʱ���кϲ� */
        __hoit_refresh_write_buffer(pInodeInfo);
    }

    return LW_TRUE;
}

/*********************************************************************************************************
** ��������: __hoit_del_write_entry
** ��������: 
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_del_write_entry(PHOIT_WRITE_BUFFER pWriteBuffer, PHOIT_WRITE_ENTRY pWriteEntry) {
    if (pWriteEntry == LW_NULL) {
        return LW_FALSE;
    }
    if (pWriteEntry->pPrev) {
        pWriteEntry->pPrev->pNext = pWriteEntry->pNext;
    }
    if (pWriteEntry->pNext) {
        pWriteEntry->pNext->pPrev = pWriteEntry->pPrev;
    }

    if (pWriteBuffer->pList == pWriteEntry) {
        pWriteBuffer->pList = pWriteEntry->pNext;
    }

    __SHEAP_FREE(pWriteEntry);

    return LW_TRUE;
}

/*********************************************************************************************************
** ��������: __hoit_refresh_write_buffer
** ��������: ��WriteBuffer�е����е����е����ݽ������ںϲ�
** �䡡��  : 
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_refresh_write_buffer(PHOIT_INODE_INFO pInodeInfo) {
    PHOIT_WRITE_BUFFER pWriteBuffer = pInodeInfo->HOITN_pWriteBuffer;
    INT32   i;
    if (pWriteBuffer == LW_NULL) {
        return LW_FALSE;
    }
    /* ������ */
    for (i = 0; i < pWriteBuffer->size-1; i++) {
        PHOIT_WRITE_ENTRY pNowEntry = pWriteBuffer->pList;
        PHOIT_WRITE_ENTRY pNextEntry = LW_NULL;
        for (; pNowEntry; pNowEntry = pNowEntry->pNext)
        {
            pNextEntry = pNowEntry->pNext;
            if (pNextEntry) {
                if (pNowEntry->pTreeNode->uiOfs > pNextEntry->pTreeNode->uiOfs) {
                    PHOIT_FRAG_TREE_NODE pTempNode = pNowEntry->pTreeNode;
                    pNowEntry->pTreeNode = pNextEntry->pTreeNode;
                    pNextEntry->pTreeNode = pTempNode;
                }
            }
            
        }
    }

    /* TODO */
    PHOIT_WRITE_ENTRY pNowEntry = pWriteBuffer->pList;
    UINT32 left     = -1;
    UINT32 right    = -1;
    UINT32 count    = 0;
    for (; pNowEntry; pNowEntry = pNowEntry->pNext)
    {
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
                    PHOIT_WRITE_ENTRY pLastEntry = pNowEntry;
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
        PHOIT_WRITE_ENTRY pLastEntry = pNowEntry;
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
