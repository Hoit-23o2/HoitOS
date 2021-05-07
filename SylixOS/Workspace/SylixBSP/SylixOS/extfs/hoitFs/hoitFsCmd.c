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
#include "hoitFsCmd.h"
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
static PHOIT_VOLUME _G_Volumn;

INT hln_cmd_wrapper(INT  iArgC, PCHAR  ppcArgV[]) {
    PCHAR pLinkDstName1 = *(ppcArgV + 1);       /* Ŀ�������ļ� */
    PCHAR pLinkSrcName2 = *(ppcArgV + 2);
    
    __hoitFsHardlink(_G_Volumn, pLinkSrcName2, pLinkDstName1);
    int fd = open("HHH", O_RDWR|O_CREAT);
    write(fd, "123\n", 4);
    return 0;
}



VOID register_hoitfs_cmd(PHOIT_VOLUME pfs) {
    _G_Volumn = pfs;
    API_TShellKeywordAdd("hln", hln_cmd_wrapper);
}
