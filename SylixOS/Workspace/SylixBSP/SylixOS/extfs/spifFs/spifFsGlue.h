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
** 文   件   名: spifFsGlue.h
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 09日
**
** 描        述: Spiffs文件系统胶水层，即上层实现
*********************************************************************************************************/

#ifndef SYLIXOS_EXTFS_SPIFFS_SPIFFSGLUE_H_
#define SYLIXOS_EXTFS_SPIFFS_SPIFFSGLUE_H_
#include "spifFsType.h"
/*********************************************************************************************************
 * SPIFFS 上层函数定义
*********************************************************************************************************/
INT32 __spiffs_mount(PSPIFFS_VOLUME pfs, PSPIFFS_CONFIG pConfig, PUCHAR pucWorkBuffer,
                     UINT8 *puiFdSpace, UINT32 uiFdSpaceSize,
                     PUCHAR pCache, UINT32 uiCacheSize,
                     spiffsCheckCallback checkCallbackFunc);

VOID __spiffs_unmount(PSPIFFS_VOLUME pfs);


#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFFSGLUE_H_ */
