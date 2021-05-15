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
** 文   件   名: hoitCmd.c
**
** 创   建   人: Hu Zhisheng
**
** 文件创建日期: 2021 年 05 月 02 日
**
** 描        述: Hoit文件系统注册硬链接shell命令
*********************************************************************************************************/
#include "hoitFsCmd.h"
#include "hoitFs.h"
/*********************************************************************************************************
** 函数名称: hln_cmd_wrppaer
** 功能描述: 注册SylixOS ttinyShell命令函数
** 输　入  : iArgC         变量数
**           ppcArgV       变量内容
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#include "./hoitFsTestFunc.h"
static PHOIT_VOLUME _G_Volumn;

#define DIVIDER                         "================="
#define GET_ARG(i)                      *(ppcArgV + i)
#define EQU_ARG(pcTargetArg, pcSrcArg)  lib_strcmp(pcTargetArg, pcSrcArg) == 0
#define FILE_MODE                       (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

#define BIG_FILE                        "RealBigFiles"



#define COLOR_RED                       "\033[31;m"
#define COLOR_GREEN                     "\033[32;m"
#define COLOR_END                       "\033[0m"
#define NEXT_LINE                       "\n"


VOID __hoitShowSectorInfo(PHOIT_VOLUME pfs){
    PHOIT_ERASABLE_SECTOR       pErasableSectorTraverse;
    pErasableSectorTraverse = pfs->HOITFS_erasableSectorList;
    while (pErasableSectorTraverse)
    {
        printf(DIVIDER "SECTOR %d" DIVIDER NEXT_LINE, pErasableSectorTraverse->HOITS_bno);
        printf("UsedSize: %d" NEXT_LINE, pErasableSectorTraverse->HOITS_uiUsedSize);
        printf("FreeSize: %d" NEXT_LINE, pErasableSectorTraverse->HOITS_uiFreeSize);
        pErasableSectorTraverse = pErasableSectorTraverse->HOITS_next;
    }
}


INT hln_cmd_wrapper(INT  iArgC, PCHAR  ppcArgV[]) {
    PCHAR pLinkDstName1 = GET_ARG(1);       /* 目标链接文件 */
    PCHAR pLinkSrcName2 = GET_ARG(2);
    
    __hoitFsHardlink(_G_Volumn, pLinkSrcName2, pLinkDstName1);
    return 0;
}

INT gc_cmd_wrapper(INT  iArgC, PCHAR  ppcArgV[]) {
    PCHAR pcGCOption;
    INT   iFd;
    UINT  i, j;
    UINT  uiSizeWritten;
    PCHAR pcWriteBuffer;

    pcGCOption = GET_ARG(1);
    if(EQU_ARG("-c", pcGCOption)){
        hoitGCClose(_G_Volumn);
    }
    else if (EQU_ARG("-t", pcGCOption))
    {
        iFd = open(BIG_FILE, O_RDWR | O_CREAT | O_TRUNC, FILE_MODE);
        if(iFd < 0){
            printf("[Create " BIG_FILE "Fail]");
            return;
        }
        uiSizeWritten = 0;
        /* 写入 64 * 26 * 1024B */
        for (i = 0; i < 64; i++)
        {
            if(i == 2){
                printf("arrive here" NEXT_LINE);
            }
            pcWriteBuffer = (PCHAR)lib_malloc(26 * 1024);
            printf("start cycle %d" NEXT_LINE, i);
            for (j = 0; j < 26 * 1024; j++)
            {
                *(pcWriteBuffer + j) = 'a';
            }
            write(iFd, pcWriteBuffer, 26 * 1024);
            uiSizeWritten += 26;
            printf("write cycle %d ok, %dKB has written, now sector is %d" NEXT_LINE, i, uiSizeWritten, 
                    _G_Volumn->HOITFS_now_sector->HOITS_bno);
            lib_free(pcWriteBuffer);
        }
        
        printf(COLOR_GREEN "Write BigFile OK" COLOR_END NEXT_LINE);
        close(iFd);
    }
}


INT fs_cmd_wrapper(INT  iArgC, PCHAR  ppcArgV[]) {
    PCHAR pcFSOption;
    pcFSOption = GET_ARG(1);
    if(EQU_ARG("-i", pcFSOption)){
        __hoitShowSectorInfo(_G_Volumn);
    }
}


VOID register_hoitfs_cmd(PHOIT_VOLUME pfs) {
    _G_Volumn = pfs;
    API_TShellKeywordAdd("hln", hln_cmd_wrapper);
    API_TShellKeywordAdd("gc", gc_cmd_wrapper);
    API_TShellKeywordAdd("hoit", fs_cmd_wrapper);
    API_TShellKeywordAdd("ftt", FileTreeTest);
    API_TShellKeywordAdd("fot", FileOverWriteTest);
}
