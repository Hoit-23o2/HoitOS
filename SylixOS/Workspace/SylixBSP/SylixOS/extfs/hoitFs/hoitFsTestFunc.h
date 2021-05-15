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
** 文   件   名: hoitFsMid.h
**
** 创   建   人: 张楠
**
** 文件创建日期: 2021 年 05 月 15 日
**
** 描        述: 测试函数，用于测试hoitfs文件系统
*********************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/shell/include/ttiny_shell.h"
#include "../SylixOS/include/sys/ioctl.h"

INT FileTreeTest (INT  iArgC, PCHAR  ppcArgV[]);
void createDir(PUCHAR pFileName, UINT dirNo);
void createFile(PUCHAR pFileName, UINT fileNo);
void FileTreeTestStart(PUCHAR pFileName);

INT FileOverWriteTest (INT  iArgC, PCHAR  ppcArgV[]);