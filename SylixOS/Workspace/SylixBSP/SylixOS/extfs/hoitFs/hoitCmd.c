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
** ��   ��   ��: hoitCmd.c
**
** ��   ��   ��: Hu Zhisheng
**
** �ļ���������: 2021 �� 05 �� 02 ��
**
** ��        ��: Hoit�ļ�ϵͳע��Ӳ����shell����
*********************************************************************************************************/
#include "hoitCmd.h"
#include "hoitFs.h"
/*********************************************************************************************************
** ��������: hln_cmd_wrppaer
** ��������: ע��SylixOS ttinyShell�����
** �䡡��  : iArgC         ������
**           ppcArgV       ��������
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_VOLUME forTest;
INT hln_cmd_wrapper(INT  iArgC, PCHAR  ppcArgV[]) {
    PCHAR pLinkDstName1 = *(ppcArgV + 1);       /* Ŀ�������ļ� */
    PCHAR pLinkSrcName2 = *(ppcArgV + 2);
    
    __hoitFsHardlink(forTest, pLinkSrcName2, pLinkDstName1);
    return 0;
}



VOID register_hln_cmd(PHOIT_VOLUME pfs) {
    forTest = pfs;
    API_TShellKeywordAdd("hln", hln_cmd_wrapper);
}
