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
** ��   ��   ��: HoitWriteBuffer.h
**
** ��   ��   ��: Hu Zhisheng
**
** �ļ���������: 2021 �� 07 �� 10 ��
**
** ��        ��: Hoit�ļ�ϵͳΪÿ���ļ�������WriteBuffer�ĺ�����, ���ںϲ�д��С����
*********************************************************************************************************/
#ifndef __HOITFSWRITEBUFFER_H
#define __HOITFSWRITEBUFFER_H

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL

#include "SylixOS.h"
#include "hoitType.h"
/*
*	�����趨WriteBuffer����ֵΪ16���ڵ㣬��ÿ���ڵ㶼��������С��16B��;
*	Ŀǰ����open����inodeinfoʱ��ʼ��writebuffer����дһ������ʱ���С���ݲ����뵽writebuffer;
*	ͬʱ�ں�����ڵ�fixɾ���ڵ�ʱ�ǵ�ɾ����ָָ��;
*/
#define HOIT_WRITE_BUFFER_THRESHOLD 16	// WriteBuffer�����ϲ������Ľڵ���
#define HOIT_WRITE_BUFFER_FRAGSIZE	16

BOOL __hoit_new_write_buffer(PHOIT_INODE_INFO pInodeInfo);
BOOL __hoit_new_write_entry(PHOIT_INODE_INFO pInodeInfo, PHOIT_WRITE_BUFFER pWriteBuffer, PHOIT_FRAG_TREE_NODE pTreeNode);
BOOL __hoit_del_write_entry(PHOIT_WRITE_BUFFER pWriteBuffer, PHOIT_WRITE_ENTRY pWriteEntry);
BOOL __hoit_refresh_write_buffer(PHOIT_INODE_INFO pInodeInfo);
BOOL __hoit_free_write_buffer(PHOIT_INODE_INFO pInodeInfo);

#endif
