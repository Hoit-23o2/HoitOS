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
** 文   件   名: spifFsLib.h
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 01日
**
** 描        述: Spiffs文件系统核心
*********************************************************************************************************/
#ifndef SYLIXOS_EXTFS_SPIFFS_SPIFFSLIB_H_
#define SYLIXOS_EXTFS_SPIFFS_SPIFFSLIB_H_
#include "spifFsType.h"
/*********************************************************************************************************
 * SPIFFS Vistor相关定义
*********************************************************************************************************/
// check id, only visit matching objec ids
#define SPIFFS_VIS_CHECK_ID     (1<<0)
// report argument object id to visitor - else object lookup id is reported
#define SPIFFS_VIS_CHECK_PH     (1<<1)
// stop searching at end of all look up pages
#define SPIFFS_VIS_NO_WRAP      (1<<2)

// visitor result, continue searching
#define SPIFFS_VIS_COUNTINUE            (SPIFFS_ERR_INTERNAL - 20)
// visitor result, continue searching after reloading lu buffer
#define SPIFFS_VIS_COUNTINUE_RELOAD     (SPIFFS_ERR_INTERNAL - 21)
// visitor result, stop searching
#define SPIFFS_VIS_END                  (SPIFFS_ERR_INTERNAL - 22)


typedef INT32 (*spiffsVisitorFunc)(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID objId, SPIFFS_BLOCK_IX blkIX, INT iLookUpEntryIX, 
                                   const PVOID pUserConst, PVOID pUserVar);

/*********************************************************************************************************
 * SPIFFS 库函数定义
*********************************************************************************************************/
INT32 spiffsObjLookUpScan(PSPIFFS_VOLUME pfs);
INT32 spiffsObjLookUpFindEntryVisitor(PSPIFFS_VOLUME pfs, SPIFFS_BLOCK_IX blkIXStarting, INT iLookUpEntryStarting,
                                      UINT8 flags, SPIFFS_OBJ_ID objId, spiffsVisitorFunc vistor,
                                      const PVOID pUserConst, PVOID pUserVar, SPIFFS_BLOCK_IX *pBlkIX, INT *piLookUpEntry);
INT32 spiffsObjLookUpFindFreeObjId(PSPIFFS_VOLUME pfs, SPIFFS_OBJ_ID *pObjId, const PUCHAR pucConflictingName);

#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFFSLIB_H_ */
