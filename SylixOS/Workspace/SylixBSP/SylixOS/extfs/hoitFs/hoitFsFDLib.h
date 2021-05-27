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
** ��   ��   ��: HoitFsFDLib.h
**
** ��   ��   ��: Hu Zhisheng
**
** �ļ���������: 2021 �� 04 �� 10 ��
**
** ��        ��: Hoit�ļ�ϵͳ�ٿ�FullDnode�ĺ�����.
*********************************************************************************************************/
#ifndef __HOITFSFDLIB_H
#define __HOITFSFDLIB_H

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
//#include "../SylixOS/kernel/include/k_kernel.h"
//#include "../SylixOS/system/include/s_system.h"
//#include "../SylixOS/fs/include/fs_fs.h"
#include "SylixOS.h"
#include "hoitType.h"


BOOL                __hoit_delete_full_dnode(PHOIT_VOLUME pfs, PHOIT_FULL_DNODE pFullDnode, INT flag);
BOOL                __hoit_update_full_dnode(PHOIT_FULL_DNODE pFullDnode, UINT offset, UINT length);
PHOIT_FULL_DNODE    __hoit_truncate_full_dnode(PHOIT_VOLUME pfs, PHOIT_FULL_DNODE pFullDnode, UINT offset, UINT length);
PHOIT_FULL_DNODE    __hoit_write_full_dnode(PHOIT_INODE_INFO pInodeInfo, UINT offset, UINT size, PCHAR pContent, UINT needLog);
PHOIT_FULL_DNODE    __hoit_bulid_full_dnode(PHOIT_VOLUME pfs, PHOIT_RAW_INFO pRawInfo);
PHOIT_FULL_DIRENT   __hoit_bulid_full_dirent(PHOIT_VOLUME pfs, PHOIT_RAW_INFO pRawInfo);

#endif
