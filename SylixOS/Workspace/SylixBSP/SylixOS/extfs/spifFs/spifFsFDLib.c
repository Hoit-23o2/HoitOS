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
** 文   件   名: spifFsFDLib.h
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 10日
**
** 描        述: Spiffs文件句柄管理库
*********************************************************************************************************/

#include "spifFsFDLib.h"

INT32 spiffsFdGet(PSPIFFS_VOLUME pfs, SPIFFS_FILE file, PSPIFFS_FD *pfd) {
  if (file <= 0 || file > (INT16)pfs->uiFdCount) {
    return SPIFFS_ERR_BAD_DESCRIPTOR;
  }
  PSPIFFS_FD pFds = (PSPIFFS_FD)pfs->pucFdSpace;
  *pfd = &pFds[file - 1];
  if ((*pfd)->fileN == 0) {
    return SPIFFS_ERR_FILE_CLOSED;
  }
  return SPIFFS_OK;
}
