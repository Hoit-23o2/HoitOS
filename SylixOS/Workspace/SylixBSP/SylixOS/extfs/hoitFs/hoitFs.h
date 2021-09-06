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
** 文   件   名: HoitFs.h
**
** 创   建   人: Hoit Group
**
** 文件创建日期: 2021 年 03 月 19 日
**
** 描        述: HoitFs
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
// #define HOITFS_DISABLE
#ifndef HOITFS_DISABLE


#ifndef __HOITFS_H
#define __HOITFS_H
#include "hoitType.h"
/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0// && LW_CFG_RAMFS_EN > 0
/*********************************************************************************************************
  API
*********************************************************************************************************/
LW_API INT          API_HoitFsDrvInstall(VOID);
LW_API INT          API_HoitFsDevCreate(PCHAR   pcName, PLW_BLK_DEV  pblkd);
LW_API INT          API_HoitFsDevDelete(PCHAR   pcName);

INT                 __hoitFsHardlink(PHOIT_VOLUME pfs, PCHAR pcName, CPCHAR pcLinkDst);

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
#endif                                                                  /*  __HOITFS_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/
#endif // HOITFS_DISABLE
