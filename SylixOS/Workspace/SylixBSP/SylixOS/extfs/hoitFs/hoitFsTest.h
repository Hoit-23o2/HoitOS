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
#include "hoitType.h"

INT     hoitTestFileTree (INT  iArgC, PCHAR  ppcArgV[]);
INT     hoitTestFileOverWrite (INT  iArgC, PCHAR  ppcArgV[]);
INT     hoitTestLink (INT  iArgC, PCHAR  ppcArgV[]);
INT     hoitTestGC(PHOIT_VOLUME pfs);
INT     hoitEBSTest(PHOIT_VOLUME pfs);
INT     hoitEBSCheckCmd(PHOIT_VOLUME pfs, INT  iArgC, PCHAR  ppcArgV[]);
VOID    hoitGetRawInfoMemCost(PHOIT_VOLUME pfs);
