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
** ��   ��   ��: hoitFsCmd.c
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
#include "./hoitFsTest.h"
#include "./hoitFsCache.h"
PHOIT_VOLUME _G_Volumn;

#define DIVIDER                              "================="
#define GET_ARG(i)                           *(ppcArgV + i)
#define EQU_ARG(pcTargetArg, pcSrcArg)       lib_strcmp(pcTargetArg, pcSrcArg) == 0
#define STARTWITH_ARG(pcTargetArg, pcSrcArg) lib_memcmp(pcTargetArg, pcSrcArg, lib_strlen(pcSrcArg)) == 0

#define NEXT_LINE                            "\n"

/* //!Added By PYQ 2021-09-04 ��ӽ����ؼ�����غ꣬������ */
#define DEFINE_KEYWORD(keyword)              PCHAR KEYWORD_##keyword = "$"#keyword;
#define IS_KEYWORD(token, keyword)           lib_strcmp(token, KEYWORD_##keyword) == 0

#define ARG_FEATURE_ON                       "on"
#define ARG_FEATURE_OFF                      "off"

PHOIT_VOLUME _G_Volumn;

VOID __hoitShowSectorInfo(PHOIT_VOLUME pfs){
    PHOIT_ERASABLE_SECTOR       pErasableSectorTraverse;
    pErasableSectorTraverse = pfs->HOITFS_erasableSectorList;
    while (pErasableSectorTraverse)
    {
        API_TShellColorStart2(LW_TSHELL_COLOR_GREEN, STD_OUT);
        if(hoitLogCheckIfLog(pfs, pErasableSectorTraverse)){
            printf(DIVIDER "SECTOR %2d [*LOG] " DIVIDER NEXT_LINE, pErasableSectorTraverse->HOITS_bno);
        }
        else if(pErasableSectorTraverse == pfs->HOITFS_now_sector){
            printf(DIVIDER "SECTOR %2d [*CUR] " DIVIDER NEXT_LINE, pErasableSectorTraverse->HOITS_bno);
        }
        else {
            printf(DIVIDER "SECTOR %2d" DIVIDER NEXT_LINE, pErasableSectorTraverse->HOITS_bno);
        }
        printf("UsedSize: %d" NEXT_LINE, pErasableSectorTraverse->HOITS_uiUsedSize);
        printf("FreeSize: %d" NEXT_LINE, pErasableSectorTraverse->HOITS_uiFreeSize);
        printf("ObseletEntity:   %d" NEXT_LINE, pErasableSectorTraverse->HOITS_uiObsoleteEntityCount);
        printf("AvailableEntity: %d" NEXT_LINE, pErasableSectorTraverse->HOITS_uiAvailableEntityCount);
        pErasableSectorTraverse = pErasableSectorTraverse->HOITS_next;
        API_TShellColorEnd(STD_OUT);
    }
    API_TShellColorStart2(LW_TSHELL_COLOR_CYAN, STD_OUT);
                   
    printf(DIVIDER "MORE INFO" DIVIDER NEXT_LINE, pErasableSectorTraverse->HOITS_bno);
    printf("Foreground GC Times: %ld" NEXT_LINE, pfs->ulGCForegroundTimes);
    printf("Background GC Times: %ld" NEXT_LINE, pfs->ulGCBackgroundTimes);
    printf("Cur Memory Cost    : %u" NEXT_LINE, pfs->HOITFS_ulCurBlk);
    printf("Max Memory Cost    : %u" NEXT_LINE, pfs->HOITFS_ulMaxBlk);
    API_TShellColorEnd(STD_OUT);
}


INT hln_cmd_wrapper(INT  iArgC, PCHAR  ppcArgV[]) {
    PCHAR pLinkDstName1 = GET_ARG(1);       /* Ŀ�������ļ� */
    PCHAR pLinkSrcName2 = GET_ARG(2);
    
    __hoitFsHardlink(_G_Volumn, pLinkSrcName2, pLinkDstName1);
    return 0;
}

INT gc_cmd_wrapper(INT  iArgC, PCHAR  ppcArgV[]) {
    PCHAR pcGCOption;

    pcGCOption = GET_ARG(1);
    if(EQU_ARG("-c", pcGCOption)){
        hoitGCClose(_G_Volumn);
    }
    else if (EQU_ARG("-t", pcGCOption))
    {
        hoitTestGC(_G_Volumn);
        printf("\n");
    }
}

INT fs_cmd_wrapper(INT  iArgC, PCHAR  ppcArgV[]) {
    PCHAR       pcFSOption;
    
    pcFSOption = GET_ARG(1);
    if(EQU_ARG("-i", pcFSOption)){
        __hoitShowSectorInfo(_G_Volumn);
    }
    else if (EQU_ARG("-t", pcFSOption))
    {
        pcFSOption = GET_ARG(2);
        if(EQU_ARG("ftt", pcFSOption)){                                 /* hoit -t ftt 2 3 2 */
            hoitTestFileTree(iArgC - 2, ppcArgV + 2);
        }
        else if (EQU_ARG("fot", pcFSOption))                            /* hoit -t fot */
        {
            hoitTestFileOverWrite(iArgC - 2, ppcArgV + 2);
        }
        else if (EQU_ARG("ln", pcFSOption))
        {
            hoitTestLink(iArgC - 2, ppcArgV + 2);
        }
        else if (EQU_ARG("ebs", pcFSOption)) {
            //hoitEBSTest(_G_Volumn);
            hoitEBSCheckCmd(_G_Volumn, iArgC - 2, ppcArgV + 2);
        }
        else if (EQU_ARG("raw", pcFSOption)) {
            hoitGetRawInfoMemCost(_G_Volumn);
        }
    }
}

VOID register_hoitfs_cmd(PHOIT_VOLUME pfs) {
    _G_Volumn = pfs;
    API_TShellKeywordAdd("hln", hln_cmd_wrapper);
    API_TShellKeywordAdd("gc", gc_cmd_wrapper);
    API_TShellKeywordAdd("hoit", fs_cmd_wrapper);
}



/*********************************************************************************************************
  ���parser����
*********************************************************************************************************/
VOID __parse_CRC(PHOIT_VOLUME pfs, PCHAR arg);
VOID __parse_EBS(PHOIT_VOLUME pfs, PCHAR arg);
VOID __parse_MT(PHOIT_VOLUME pfs, PCHAR arg);
VOID __parse_MTREE(PHOIT_VOLUME pfs, PCHAR arg);
VOID __parse_BGC(PHOIT_VOLUME pfs, PCHAR arg);
VOID __parse_TREE(PHOIT_VOLUME pfs, PCHAR arg);
VOID __parse_OPTION(PHOIT_VOLUME pfs, PCHAR arg);
/*********************************************************************************************************
  parserʵ��
*********************************************************************************************************/
VOID __parse_CRC(PHOIT_VOLUME pfs, PCHAR arg){
    if(EQU_ARG(arg, ARG_FEATURE_ON)){
        pfs->HOITFS_config.HOITFS_CRC_bEnableCRCDataCheck = LW_TRUE;
    }
    else if(EQU_ARG(arg, ARG_FEATURE_OFF)){
        pfs->HOITFS_config.HOITFS_CRC_bEnableCRCDataCheck = LW_FALSE;
    }
}

VOID __parse_EBS(PHOIT_VOLUME pfs, PCHAR arg){
    if(EQU_ARG(arg, ARG_FEATURE_ON)){
        pfs->HOITFS_config.HOITFS_EBS_bEnableEBS = LW_TRUE;
    }
    else if(EQU_ARG(arg, ARG_FEATURE_OFF)){
        pfs->HOITFS_config.HOITFS_EBS_bEnableEBS = LW_FALSE;
    }
}

VOID __parse_MT(PHOIT_VOLUME pfs, PCHAR arg){
    CHAR cBufferKey[LW_CFG_SHELL_MAX_COMMANDLEN] = {0};
    CHAR cBufferValue[LW_CFG_SHELL_MAX_COMMANDLEN] = {0};
    if(EQU_ARG(arg, ARG_FEATURE_ON)){
        pfs->HOITFS_config.HOITFS_MT_bEnableMultiThreadScan = LW_TRUE;
    }
    else if(EQU_ARG(arg, ARG_FEATURE_OFF)){
        pfs->HOITFS_config.HOITFS_MT_bEnableMultiThreadScan = LW_FALSE;
    }
    else if(STARTWITH_ARG(arg, "tcnt")){
        sscanf(arg, "%[^=]=%s", cBufferKey, cBufferValue);
        pfs->HOITFS_config.HOITFS_MT_uiThreadCnt = lib_atoi(cBufferValue);
    }
}

VOID __parse_MTREE(PHOIT_VOLUME pfs, PCHAR arg){
    CHAR cBufferKey[LW_CFG_SHELL_MAX_COMMANDLEN] = {0};
    CHAR cBufferValue[LW_CFG_SHELL_MAX_COMMANDLEN] = {0};
    if(EQU_ARG(arg, ARG_FEATURE_ON)){
        pfs->HOITFS_config.HOITFS_MTREE_bEnableMergeBuffer = LW_TRUE;
    }
    else if(EQU_ARG(arg, ARG_FEATURE_OFF)){
        pfs->HOITFS_config.HOITFS_MTREE_bEnableMergeBuffer = LW_FALSE;
    }
    else if(STARTWITH_ARG(arg, "mdatasz")){
        sscanf(arg, "%[^=]=%s", cBufferKey, cBufferValue);
        pfs->HOITFS_config.HOITFS_MTREE_uiMergeDataThreshold = lib_atoi(cBufferValue);
    }
    else if(STARTWITH_ARG(arg, "mbuffersz")){
        sscanf(arg, "%[^=]=%s", cBufferKey, cBufferValue);
        pfs->HOITFS_config.HOITFS_MTREE_uiMergeBufferThreshold = lib_atoi(cBufferValue);
    }
}

VOID __parse_BGC(PHOIT_VOLUME pfs, PCHAR arg){
    CHAR cBufferKey[LW_CFG_SHELL_MAX_COMMANDLEN] = {0};
    CHAR cBufferValue[LW_CFG_SHELL_MAX_COMMANDLEN] = {0};
    if(EQU_ARG(arg, ARG_FEATURE_ON)){
        pfs->HOITFS_config.HOITFS_BGC_bEnableBackgroundGC = LW_TRUE;
    }
    else if(EQU_ARG(arg, ARG_FEATURE_OFF)){
        pfs->HOITFS_config.HOITFS_BGC_bEnableBackgroundGC = LW_FALSE;
    }
    else if(STARTWITH_ARG(arg, "thr")){
        sscanf(arg, "%[^=]=%s", cBufferKey, cBufferValue);
        pfs->HOITFS_config.HOITFS_BGC_uiBackgroundGCThreshold = lib_atoi(cBufferValue);
    }
}

VOID __parse_TREE(PHOIT_VOLUME pfs, PCHAR arg){
    CHAR cBufferKey[LW_CFG_SHELL_MAX_COMMANDLEN] = {0};
    CHAR cBufferValue[LW_CFG_SHELL_MAX_COMMANDLEN] = {0};
    if(STARTWITH_ARG(arg, "mnodesz")){
        sscanf(arg, "%[^=]=%s", cBufferKey, cBufferValue);
        pfs->HOITFS_config.HOITFS_TREE_uiMaxDataSize = lib_atoi(cBufferValue);
    }
}

VOID __parse_OPTION(PHOIT_VOLUME pfs, PCHAR arg){
    if(EQU_ARG(arg, "silence") || EQU_ARG(arg, "s")){
        pfs->HOITFS_config.HOITFS_OPTION_bIsMountSilence = LW_TRUE;
    }
}
/*********************************************************************************************************
** ��������: __parse_specific_args
** ��������: hoitfs���������������Զ���
** �䡡��  : pfs           �ļ�ϵͳ
**          parser          ������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
typedef INT (*parse_arg)(PHOIT_VOLUME pfs, PCHAR arg);
typedef parse_arg PARSE_ARG;

VOID __parse_specific_args(PHOIT_VOLUME pfs, PCHAR pcArgs,PARSE_ARG parser) {
    PCHAR   pcBuffer[LW_CFG_SHELL_MAX_PARAMNUM], p;
    INT     i = 0;
    p = strtok(pcArgs, ",");       
	while(p)
	{
		pcBuffer[i] = p;
		i++;
		p = strtok(LW_NULL, ",");   
	}  
	i--;
    for (; i >= 0; i--)
    {
        parser(pfs, pcBuffer[i]);
    }
}
/*********************************************************************************************************
** ��������: parse_hoitfs_options
** ��������: hoitfs�����ϲ����
** �䡡��  : pfs           �ļ�ϵͳ
**          pcOptions       �ϲ�����ַ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID parse_hoitfs_options(PHOIT_VOLUME pfs, PCHAR pcOptions) {
    /* mount -t hoitfs $crc [off/on] $ebs [off/on] $mt [on/off,tcnt=2] \
      $bgc [off/on,thr=50] $mtree [on/off,mdatasz=16,mbuffersz=16] \
      $tree [mnodesz=1024] $o [silence|s, []]*/
    PCHAR   pcBuffer[LW_CFG_SHELL_MAX_PARAMNUM], p;   
    INT     i = 0, iTokens = 0;
    PCHAR   pcToken;
    PCHAR   pcNextToken;
    PCHAR   pcArgs;
    BOOL    bIsTokenHasArgs;
    /* Features */
    DEFINE_KEYWORD(crc);
    DEFINE_KEYWORD(ebs);
    DEFINE_KEYWORD(mt);
    DEFINE_KEYWORD(bgc);
    DEFINE_KEYWORD(mtree);
    DEFINE_KEYWORD(tree);
    /* Options */
    DEFINE_KEYWORD(o);
    p = strtok(pcOptions, " ");       
	while(p)
	{
		pcBuffer[i] = p;
		i++;
		p = strtok(LW_NULL, " ");
	}  
    iTokens = i;
    /* ����pcBuffer */
    for (i = 0; i < iTokens; i++)
    {
        pcToken = pcBuffer[i];
        if(i + 1 == iTokens){
            bIsTokenHasArgs = LW_FALSE;
        }
        else {
            pcNextToken = pcBuffer[i + 1];
            if(*pcNextToken == '$'){
                bIsTokenHasArgs = LW_FALSE;
            }
            else {
                bIsTokenHasArgs = LW_TRUE;
            }
        }

        if(!bIsTokenHasArgs){           /* û�в����б�����Ĭ�ϲ�������������� */
            continue;
        }
        else {                          /* �У���������� */
            if(IS_KEYWORD(pcToken, crc)){
                pcArgs = pcBuffer[i + 1];
                __parse_specific_args(pfs, pcArgs, __parse_CRC);
            }
            else if(IS_KEYWORD(pcToken, ebs)){
                pcArgs = pcBuffer[i + 1];
                __parse_specific_args(pfs, pcArgs, __parse_EBS);
            }
            else if(IS_KEYWORD(pcToken, mt)){
                pcArgs = pcBuffer[i + 1];
                __parse_specific_args(pfs, pcArgs, __parse_MT);
            }
            else if(IS_KEYWORD(pcToken, mtree)){
                pcArgs = pcBuffer[i + 1];
                __parse_specific_args(pfs, pcArgs, __parse_MTREE);
            }
            else if(IS_KEYWORD(pcToken, bgc)){
                pcArgs = pcBuffer[i + 1];
                __parse_specific_args(pfs, pcArgs, __parse_BGC);
            }
            else if(IS_KEYWORD(pcToken, tree)){
                pcArgs = pcBuffer[i + 1];
                __parse_specific_args(pfs, pcArgs, __parse_TREE);
            }
            else if(IS_KEYWORD(pcToken, o)){
                pcArgs = pcBuffer[i + 1];
                __parse_specific_args(pfs, pcArgs, __parse_OPTION);
            }
            i++;
        }
    }
    
}
