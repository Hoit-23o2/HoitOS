/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: hoitFsCmd.h
**
** 创   建   人: Hu Zhisheng
**
** 文件创建日期: 2021 年 05 月 02 日
**
** 描        述: Hoit文件系统注册硬链接shell命令
*********************************************************************************************************/

#ifndef __HOITFSCMD_H
#define __HOITFSCMD_H
#include "hoitType.h"
#include "hoitFsGC.h"

VOID register_hoitfs_cmd(PHOIT_VOLUME pfs);
/* //! Added By PYQ 2021-09-04 添加上层文件系统挂载选项解析 */
VOID parse_hoitfs_options(PHOIT_VOLUME pfs, PCHAR pcOptions);

#endif