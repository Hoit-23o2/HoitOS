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
** ��   ��   ��: hoitFsTest.h
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

INT     hoitTestFileTree (INT  iArgC, PCHAR  ppcArgV[]);
INT     hoitTestFileOverWrite (INT  iArgC, PCHAR  ppcArgV[]);