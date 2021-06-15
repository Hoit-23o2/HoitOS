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
** 文   件   名: spifFs.h
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 01日
**
** 描        述: Spiffs文件系统接口层
*********************************************************************************************************/

#ifndef SYLIXOS_EXTFS_SPIFFS_SPIFFS_H_
#define SYLIXOS_EXTFS_SPIFFS_SPIFFS_H_

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
//TODO  文件卷结构体和文件节点结构体所必须的属性
//!     如果对两个结构体进行更名，注意对spifFs.c进行修改
//!     默认可以通过文件节点获取文件卷信息，所以只会调用__SPIFFS_VOL_LOCK(pspiffs)对文件系统加锁
typedef struct {
    LW_DEV_HDR          SPIFFS_devhdrHdr;                                /*  ramfs 文件系统设备头        */
    LW_OBJECT_HANDLE    SPIFFS_hVolLock;                                 /*  卷操作锁                    */
    LW_LIST_LINE_HEADER SPIFFS_plineFdNodeHeader;                        /*  fd_node 链表                */
    
    BOOL                SPIFFS_bForceDelete;                             /*  是否允许强制卸载卷          */
    BOOL                SPIFFS_bValid;
    
    uid_t               SPIFFS_uid;                                      /*  用户 id                     */
    gid_t               SPIFFS_gid;                                      /*  组   id                     */
    mode_t              SPIFFS_mode;                                     /*  文件 mode                   */
    time_t              SPIFFS_time;                                     /*  创建时间                    */
    ULONG               SPIFFS_ulCurBlk;                                 /*  当前消耗内存大小            */
    ULONG               SPIFFS_ulMaxBlk;                                 /*  最大内存消耗量              */
} SPIF_VOLUME;
typedef SPIF_VOLUME     *PSPIF_VOLUME;

typedef struct ramfs_node {
    PSPIF_VOLUME         SPIFN_pramfs;                                   /*  文件系统                    */
    
    BOOL                SPIFN_bChanged;                                  /*  文件内容是否更改            */
    mode_t              SPIFN_mode;                                      /*  文件 mode                   */
    time_t              SPIFN_timeCreate;                                /*  创建时间                    */
    time_t              SPIFN_timeAccess;                                /*  最后访问时间                */
    time_t              SPIFN_timeChange;                                /*  最后修改时间                */
    
    size_t              SPIFN_stSize;                                    /*  当前文件大小 (可能大于缓冲) */
    size_t              SPIFN_stVSize;                                   /*  lseek 出的虚拟大小          */
    
    uid_t               SPIFN_uid;                                       /*  用户 id                     */
    gid_t               SPIFN_gid;                                       /*  组   id                     */
    PCHAR               SPIFN_pcName;                                    /*  文件名称                    */
    PCHAR               SPIFN_pcLink;                                    /*  链接目标                    */
    
    PLW_LIST_LINE       SPIFN_plineBStart;                               /*  文件头                      */
    PLW_LIST_LINE       SPIFN_plineBEnd;                                 /*  文件尾                      */
    ULONG               SPIFN_ulCnt;                                     /*  文件数据块数量              */
} SPIFN_NODE;
typedef SPIFN_NODE       *PSPIFN_NODE;

#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFFS_H_ */
