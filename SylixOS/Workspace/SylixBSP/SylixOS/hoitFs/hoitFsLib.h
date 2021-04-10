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

//#ifndef __HOITFSLIB_H
//#define __HOITFSLIB_H
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"                                                    /*  操作系统                    */
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
#include "../SylixOS/system/include/s_class.h"
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
#define HOIT_ERROR          100
#define __HOIT_IS_OBSOLETE(pRawHeader)          ((pRawHeader->flag & HOIT_FLAG_OBSOLETE)    == 0)
#define __HOIT_IS_TYPE_INODE(pRawHeader)        ((pRawHeader->flag & HOIT_FLAG_TYPE_INODE)  != 0)
#define __HOIT_IS_TYPE_DIRENT(pRawHeader)       ((pRawHeader->flag & HOIT_FLAG_TYPE_DIRENT) != 0)

#define __HOIT_VOLUME_LOCK(pfs)        API_SemaphoreMPend(pfs->HOITFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __HOIT_VOLUME_UNLOCK(pfs)      API_SemaphoreMPost(pfs->HOITFS_hVolLock)

/*********************************************************************************************************
  检测路径字串是否为根目录或者直接指向设备
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)           ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))

/*********************************************************************************************************
  C语言结构体提前声明
*********************************************************************************************************/
typedef struct HOIT_VOLUME HOIT_VOLUME;
typedef struct HOIT_RAW_HEADER HOIT_RAW_HEADER;
typedef struct HOIT_RAW_INODE HOIT_RAW_INODE;
typedef struct HOIT_RAW_DIRENT HOIT_RAW_DIRENT;
typedef struct HOIT_RAW_INFO HOIT_RAW_INFO;
typedef struct HOIT_FULL_DNODE HOIT_FULL_DNODE;
typedef struct HOIT_FULL_DIRENT HOIT_FULL_DIRENT;
typedef struct HOIT_INODE_CACHE HOIT_INODE_CACHE;
typedef struct HOIT_INODE_INFO HOIT_INODE_INFO;
typedef struct HOIT_SECTOR HOIT_SECTOR;



typedef HOIT_VOLUME* PHOIT_VOLUME;
typedef HOIT_RAW_HEADER* PHOIT_RAW_HEADER;
typedef HOIT_RAW_INODE* PHOIT_RAW_INODE;
typedef HOIT_RAW_DIRENT* PHOIT_RAW_DIRENT;
typedef HOIT_RAW_INFO* PHOIT_RAW_INFO;
typedef HOIT_FULL_DNODE* PHOIT_FULL_DNODE;
typedef HOIT_FULL_DIRENT* PHOIT_FULL_DIRENT;
typedef HOIT_INODE_CACHE* PHOIT_INODE_CACHE;
typedef HOIT_INODE_INFO* PHOIT_INODE_INFO;
typedef HOIT_SECTOR* PHOIT_SECTOR;
/*********************************************************************************************************
  HoitFs super block类型
*********************************************************************************************************/
typedef struct HOIT_VOLUME{
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
    PHOIT_SECTOR            HOITFS_now_sector;
    UINT                HOITFS_highest_ino;
    PHOIT_SECTOR         HOITFS_block_list;
} HOIT_VOLUME;


/*********************************************************************************************************
  HoitFs 数据实体的公共Header类型
*********************************************************************************************************/
struct HOIT_RAW_HEADER{
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;
    mode_t              file_type;
    UINT                ino;
};


/*********************************************************************************************************
  HoitFs raw inode类型
*********************************************************************************************************/
struct HOIT_RAW_INODE{
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;
    mode_t              file_type;
    UINT                ino;
};


/*********************************************************************************************************
  HoitFs raw dirent类型
*********************************************************************************************************/
struct HOIT_RAW_DIRENT{
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;
    mode_t              file_type;
    UINT                ino;
    UINT                pino;
};


struct HOIT_RAW_INFO{
    UINT                phys_addr;
    UINT                totlen;
    PHOIT_RAW_INFO      next_phys;
};


struct HOIT_FULL_DNODE{

};


struct HOIT_FULL_DIRENT{
    PHOIT_FULL_DIRENT   HOITFD_next;
    PHOIT_RAW_INFO      HOITFD_raw_info;
    UINT                HOITFD_nhash;
    PCHAR               HOITFD_file_name;
    UINT                HOITFD_ino;                                     /* 目录项指向的文件inode number      */
    UINT                HOITFD_pino;                                    /* 该目录项所属的目录文件inode number*/
    mode_t              HOITFD_file_type;
};


struct HOIT_INODE_CACHE{
    UINT                HOITC_ino;
    PHOIT_INODE_CACHE   HOITC_next;
    PHOIT_RAW_INFO      HOITC_nodes;
    UINT                HOITC_nlink;
};


struct HOIT_INODE_INFO{
    mode_t              HOITN_mode;                                     /*  文件 mode                   */
    PHOIT_INODE_CACHE   HOITN_inode_cache;
    PHOIT_FULL_DIRENT   HOITN_dents;
    PHOIT_FULL_DNODE    HOITN_metadata;
    PHOIT_FULL_DNODE    HOITN_rbtree;
    PHOIT_VOLUME        HOITN_volume;
    UINT                HOITN_ino;

    uid_t               HOITN_uid;                                      /*  用户 id                     */
    gid_t               HOITN_gid;                                      /*  组   id                     */
    time_t              RAMN_timeCreate;                                /*  创建时间                    */
    time_t              RAMN_timeAccess;                                /*  最后访问时间                */
    time_t              RAMN_timeChange;                                /*  最后修改时间                */
    size_t              RAMN_stSize;                                    /*  当前文件大小 (可能大于缓冲) */
    size_t              RAMN_stVSize;                                   /*  lseek 出的虚拟大小          */
};



struct HOIT_SECTOR{
    PHOIT_SECTOR        HOITS_next;
    UINT                HOITS_bno;                                      /* 块号block number              */
    UINT                HOITS_addr;
    UINT                HOITS_length;
    UINT                HOITS_offset;                                   /* 当前在写物理地址=addr+offset  */
};

PHOIT_INODE_INFO  __hoit_just_open(PHOIT_INODE_INFO pdir, PCHAR pName);
UINT __hoit_name_hash(CPCHAR pcName);
UINT __hoit_free_full_dirent(PHOIT_FULL_DIRENT pDirent);
PHOIT_INODE_INFO __hoit_get_full_file(PHOIT_VOLUME pfs, UINT ino);
PHOIT_INODE_CACHE __hoit_get_inode_cache(PHOIT_VOLUME pfs, UINT ino);
VOID __hoit_add_dirent(PHOIT_INODE_INFO pFatherInode, PHOIT_FULL_DIRENT pSonDirent);
UINT __hoit_alloc_ino(PHOIT_VOLUME pfs);
UINT8 __hoit_write_flash(PHOIT_VOLUME pfs, PVOID pdata, UINT length, UINT* phys_addr);
UINT8 __hoit_write_flash_thru(PHOIT_VOLUME pfs, PVOID pdata, UINT length, UINT phys_addr);
UINT8 __hoit_add_to_inode_cache(PHOIT_INODE_CACHE pInodeCache, PHOIT_RAW_INFO pRawInfo);
UINT8 __hoit_add_to_cache_list(PHOIT_VOLUME pfs, PHOIT_INODE_CACHE pInodeCache);
UINT8 __hoit_add_to_dents(PHOIT_INODE_INFO pInodeFather, PHOIT_FULL_DIRENT pFullDirent);
PHOIT_FULL_DIRENT __hoit_search_in_dents(PHOIT_INODE_INFO pInodeFather, UINT ino);
UINT8 __hoit_del_raw_info(PHOIT_INODE_CACHE pInodeCache, PHOIT_RAW_INFO pRawInfo);
UINT8 __hoit_del_raw_data(PHOIT_RAW_INFO pRawInfo);
UINT8 __hoit_del_full_dirent(PHOIT_INODE_INFO pInodeInfo, PHOIT_FULL_DIRENT pFullDirent);
UINT8 __hoit_del_inode_cache(PHOIT_VOLUME pfs, PHOIT_INODE_CACHE pInodeCache);

PHOIT_INODE_INFO  __hoit_open(PHOIT_VOLUME  pfs,
    CPCHAR       pcName,
    PHOIT_INODE_INFO* ppinodeFather,
    BOOL* pbRoot,
    BOOL* pbLast,
    PCHAR* ppcTail);

PHOIT_INODE_INFO  __hoit_maken(PHOIT_VOLUME  pfs,
    CPCHAR       pcName,
    PHOIT_INODE_INFO    pInodeFather,
    mode_t       mode,
    CPCHAR       pcLink);
INT  __hoit_unlink_regular(PHOIT_INODE_INFO pInodeFather, PHOIT_FULL_DIRENT  pDirent);
VOID  __hoit_truncate(PHOIT_INODE_INFO  pInodeInfo, size_t  offset);
INT  __hoit_unlink_dir(PHOIT_INODE_INFO pInodeFather, PHOIT_FULL_DIRENT  pDirent);
INT  __hoit_move_check(PHOIT_INODE_INFO  pInode1, PHOIT_INODE_INFO  pInode2);
INT  __hoit_move(PHOIT_INODE_INFO pInodeFather, PHOIT_INODE_INFO  pInodeInfo, PCHAR  pcNewName);
INT  __hoit_stat(PHOIT_INODE_INFO pInodeInfo, PHOIT_VOLUME  pfs, struct stat* pstat);
INT  __hoit_statfs(PHOIT_VOLUME  pfs, struct statfs* pstatfs);
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0       */
//#endif                                                                  /*  __HOITFSLIB_H                */
