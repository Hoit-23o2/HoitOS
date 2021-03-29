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
** 文   件   名: HoitFsLib.h
**
** 创   建   人: Hoit Group
**
** 文件创建日期: 2021 年 03 月 20 日
**
** 描        述: Hoit文件系统内部函数.
*********************************************************************************************************/

#ifndef __HOITFSLIB_H
#define __HOITFSLIB_H

#include "SylixOS.h"                                                    /*  操作系统                    */

/*********************************************************************************************************
  裁剪宏
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0 //&& LW_CFG_RAMFS_EN > 0

/*********************************************************************************************************
  HoitFs宏定义
*********************************************************************************************************/
#define HOIT_MAGIC_NUM      0x05201314
#define HOIT_FIELD_TYPE     0xE0000000          //前3位作为TYPE域

#define HOIT_FLAG_TYPE_INODE     0x20000000     //raw_inode类型 001
#define HOIT_FLAG_TYPE_DIRENT    0x40000000     //raw_dirent类型  010
#define HOIT_FLAG_OBSOLETE       0x00000001     //Flag的最后一位用来表示是否过期，1是没过期，0是过期
#define HOIT_ERROR          500

#define __HOIT_SB_LOCK(pfs)        API_SemaphoreMPend(pfs->HOITFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __HOIT_SB_UNLOCK(pfs)      API_SemaphoreMPost(pfs->HOITFS_hVolLock)

/*********************************************************************************************************
  检测路径字串是否为根目录或者直接指向设备
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)           ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))

/*********************************************************************************************************
  HoitFs super block类型
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR          HOITFS_devhdrHdr;                                /*  HoitFs 文件系统设备头        */
    LW_OBJECT_HANDLE    HOITFS_hVolLock;                                 /*  卷操作锁                    */
    LW_LIST_LINE_HEADER HOITFS_plineFdNodeHeader;                        /*  fd_node 链表                */
    PHOIT_INODE_INFO    HOITFS_pRootDir;                                 /*  根目录文件暂定为一直打开的  */

    BOOL                HOITFS_bForceDelete;                             /*  是否允许强制卸载卷          */
    BOOL                HOITFS_bValid;

    uid_t               HOITFS_uid;                                      /*  用户 id                     */
    gid_t               HOITFS_gid;                                      /*  组   id                     */
    mode_t              HOITFS_mode;                                     /*  文件 mode                   */
    time_t              HOITFS_time;                                     /*  创建时间                    */
    ULONG               HOITFS_ulCurBlk;                                 /*  当前消耗内存大小            */
    ULONG               HOITFS_ulMaxBlk;                                 /*  最大内存消耗量              */

    PHOIT_INODE_CACHE   HOITFS_cache_list;
    PHOIT_BLOCK         HOITFS_now_block;
    UINT                HOITFS_highest_ino;
} HOIT_VOLUME;
typedef HOIT_VOLUME* PHOIT_VOLUME;

/*********************************************************************************************************
  HoitFs 数据实体的公共Header类型
*********************************************************************************************************/
typedef struct {
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;
    mode_t              file_type;
} HOIT_RAW_HEADER;
typedef HOIT_RAW_HEADER* PHOIT_RAW_HEADER;
/*********************************************************************************************************
  HoitFs raw inode类型
*********************************************************************************************************/
typedef struct {
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;
    mode_t              file_type;
    UINT                ino;
} HOIT_RAW_INODE;
typedef HOIT_RAW_INODE* PHOIT_RAW_INODE;

/*********************************************************************************************************
  HoitFs raw dirent类型
*********************************************************************************************************/
typedef struct {
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;
    mode_t              file_type;
    UINT                ino;
    UINT                pino;
} HOIT_RAW_DIRENT;
typedef HOIT_RAW_DIRENT* PHOIT_RAW_DIRENT;

typedef struct {
    UINT                phys_addr;
    UINT                totlen;
    PHOIT_RAW_INFO      next_phys;
}HOIT_RAW_INFO;
typedef HOIT_RAW_INFO* PHOIT_RAW_INFO;

typedef struct {

}HOIT_FULL_DNODE;
typedef HOIT_FULL_DNODE* PHOIT_FULL_DNODE;

typedef struct {
    PHOIT_FULL_DIRENT   HOITFD_next;
    PHOIT_RAW_INFO      HOITFD_raw_info;
    UINT                HOITFD_nhash;
    PCHAR               HOITFD_file_name;
    UINT                HOITFD_ino;                                      /*  文件inode number            */
    UINT                HOITFD_pino;
    mode_t              HOITFD_file_type;
}HOIT_FULL_DIRENT;
typedef HOIT_FULL_DIRENT* PHOIT_FULL_DIRENT;

typedef struct {
    UINT                HOITC_ino;
    PHOIT_INODE_CACHE   HOITC_next;
    PHOIT_RAW_INFO      HOITC_nodes;
    UINT                HOITC_nlink;
}HOIT_INODE_CACHE;
typedef HOIT_INODE_CACHE* PHOIT_INODE_CACHE;

typedef struct {
    mode_t              HOITN_mode;                                      /*  文件 mode                   */
    PHOIT_INODE_CACHE   HOITN_inode_cache;
    PHOIT_FULL_DIRENT   HOITN_dents;
    PHOIT_FULL_DNODE    HOITN_metadata;
    PHOIT_FULL_DNODE    HOITN_rbtree;
    PHOIT_VOLUME        HOITN_volume;
    UINT                HOITN_ino;

    uid_t               HOITN_uid;                                       /*  用户 id                     */
    gid_t               HOITN_gid;                                       /*  组   id                     */
    time_t              RAMN_timeCreate;                                /*  创建时间                    */
    time_t              RAMN_timeAccess;                                /*  最后访问时间                */
    time_t              RAMN_timeChange;                                /*  最后修改时间                */
    size_t              RAMN_stSize;                                    /*  当前文件大小 (可能大于缓冲) */
    size_t              RAMN_stVSize;                                   /*  lseek 出的虚拟大小          */
}HOIT_INODE_INFO;
typedef HOIT_INODE_INFO* PHOIT_INODE_INFO;


typedef struct {
    UINT HOITB_addr;
    UINT HOITB_length;
    UINT HOITB_offset;                                                  /* 当前在写物理地址=addr+offset  */
} HOIT_BLOCK;
typedef HOIT_BLOCK* PHOIT_BLOCK;

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0       */
#endif                                                                  /*  __HOITFSLIB_H                */
