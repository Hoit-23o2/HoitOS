/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: hoitFsMid.h
**
** ��   ��   ��: ���
**
** �ļ���������: 2021 �� 05 �� 15 ��
**
** ��        ��: ���Ժ��������ڲ���hoitfs�ļ�ϵͳ
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