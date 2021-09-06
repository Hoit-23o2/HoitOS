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
** ��   ��   ��: HoitMergeBuffer.h
**
** ��   ��   ��: Hu Zhisheng
**
** �ļ���������: 2021 �� 07 �� 10 ��
**
** ��        ��: Hoit�ļ�ϵͳΪÿ���ļ�������MergeBuffer�ĺ�����, ���ںϲ�д��С����
*********************************************************************************************************/
#ifndef __HOITFSMERGEBUFFER_H
#define __HOITFSMERGEBUFFER_H

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL

#include "SylixOS.h"
#include "hoitType.h"
/*
*	�����趨MergeBuffer����ֵΪ16���ڵ㣬��ÿ���ڵ㶼��������С��16B��;
*	Ŀǰ����open����inodeinfoʱ��ʼ��MergeBuffer����дһ������ʱ���С���ݲ����뵽MergeBuffer;
*	ͬʱ�ں�����ڵ�fixɾ���ڵ�ʱ�ǵ�ɾ����ָָ��;
*/


BOOL __hoit_new_merge_buffer(PHOIT_INODE_INFO pInodeInfo);
BOOL __hoit_new_merge_entry(PHOIT_INODE_INFO pInodeInfo, PHOIT_MERGE_BUFFER pMergeBuffer, PHOIT_FRAG_TREE_NODE pTreeNode);
BOOL __hoit_del_merge_entry(PHOIT_VOLUME pfs, PHOIT_MERGE_BUFFER pMergeBuffer, PHOIT_MERGE_ENTRY pMergeEntry);
BOOL __hoit_refresh_merge_buffer(PHOIT_INODE_INFO pInodeInfo);
BOOL __hoit_free_merge_buffer(PHOIT_INODE_INFO pInodeInfo);
BOOL __hoit_clear_merge_buffer(PHOIT_INODE_INFO pInodeInfo);

#endif
