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
#include "hoitCmd.h"
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
PHOIT_VOLUME forTest;
INT hln_cmd_wrapper(INT  iArgC, PCHAR  ppcArgV[]) {
    PCHAR pLinkDstName1 = *(ppcArgV + 1);       /* 目标链接文件 */
    PCHAR pLinkSrcName2 = *(ppcArgV + 2);
    
    __hoitFsHardlink(forTest, pLinkSrcName2, pLinkDstName1);
    return 0;
}



VOID register_hln_cmd(PHOIT_VOLUME pfs) {
    forTest = pfs;
    API_TShellKeywordAdd("hln", hln_cmd_wrapper);
}
