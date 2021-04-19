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
** ��   ��   ��: HoitFsLib.h
**
** ��   ��   ��: Hoit Group
**
** �ļ���������: 2021 �� 03 �� 20 ��
**
** ��        ��: Hoit�ļ�ϵͳ�ڲ�����.
*********************************************************************************************************/

//#ifndef __HOITFSLIB_H
//#define __HOITFSLIB_H
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"                                                    /*  ����ϵͳ                    */
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
#include "../SylixOS/system/include/s_class.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0 //&& LW_CFG_RAMFS_EN > 0

/*********************************************************************************************************
  HoitFs�궨��
*********************************************************************************************************/
#define HOIT_MAGIC_NUM      0x05201314
#define HOIT_FIELD_TYPE     0xE0000000          //ǰ3λ��ΪTYPE��

#define HOIT_FLAG_TYPE_INODE     0x20000000     //raw_inode���� 001
#define HOIT_FLAG_TYPE_DIRENT    0x40000000     //raw_dirent����  010
#define HOIT_FLAG_OBSOLETE       0x00000001     //Flag�����һλ������ʾ�Ƿ���ڣ�1��û���ڣ�0�ǹ���
#define HOIT_ERROR          100
#define __HOIT_IS_OBSOLETE(pRawHeader)          ((pRawHeader->flag & HOIT_FLAG_OBSOLETE)    == 0)
#define __HOIT_IS_TYPE_INODE(pRawHeader)        ((pRawHeader->flag & HOIT_FLAG_TYPE_INODE)  != 0)
#define __HOIT_IS_TYPE_DIRENT(pRawHeader)       ((pRawHeader->flag & HOIT_FLAG_TYPE_DIRENT) != 0)

#define __HOIT_VOLUME_LOCK(pfs)        API_SemaphoreMPend(pfs->HOITFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __HOIT_VOLUME_UNLOCK(pfs)      API_SemaphoreMPost(pfs->HOITFS_hVolLock)

/*********************************************************************************************************
  ���·���ִ��Ƿ�Ϊ��Ŀ¼����ֱ��ָ���豸
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)           ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))

/*********************************************************************************************************
  C���Խṹ����ǰ����
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
  HoitFs super block����
*********************************************************************************************************/
typedef struct HOIT_VOLUME{
    LW_DEV_HDR          HOITFS_devhdrHdr;                                /*  HoitFs �ļ�ϵͳ�豸ͷ        */
    LW_OBJECT_HANDLE    HOITFS_hVolLock;                                 /*  �������                    */
    LW_LIST_LINE_HEADER HOITFS_plineFdNodeHeader;                        /*  fd_node ����                */
    PHOIT_INODE_INFO    HOITFS_pRootDir;                                 /*  ��Ŀ¼�ļ��ݶ�Ϊһֱ�򿪵�  */

    BOOL                HOITFS_bForceDelete;                             /*  �Ƿ�����ǿ��ж�ؾ�          */
    BOOL                HOITFS_bValid;

    uid_t               HOITFS_uid;                                      /*  �û� id                     */
    gid_t               HOITFS_gid;                                      /*  ��   id                     */
    mode_t              HOITFS_mode;                                     /*  �ļ� mode                   */
    time_t              HOITFS_time;                                     /*  ����ʱ��                    */
    ULONG               HOITFS_ulCurBlk;                                 /*  ��ǰ�����ڴ��С            */
    ULONG               HOITFS_ulMaxBlk;                                 /*  ����ڴ�������              */

    PHOIT_INODE_CACHE   HOITFS_cache_list;
    PHOIT_SECTOR            HOITFS_now_sector;
    UINT                HOITFS_highest_ino;
    PHOIT_SECTOR         HOITFS_block_list;
} HOIT_VOLUME;


/*********************************************************************************************************
  HoitFs ����ʵ��Ĺ���Header����
*********************************************************************************************************/
struct HOIT_RAW_HEADER{
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;
    mode_t              file_type;
    UINT                ino;
};


/*********************************************************************************************************
  HoitFs raw inode����
*********************************************************************************************************/
struct HOIT_RAW_INODE{
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;
    mode_t              file_type;
    UINT                ino;
};


/*********************************************************************************************************
  HoitFs raw dirent����
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
    UINT                HOITFD_ino;                                     /* Ŀ¼��ָ����ļ�inode number      */
    UINT                HOITFD_pino;                                    /* ��Ŀ¼��������Ŀ¼�ļ�inode number*/
    mode_t              HOITFD_file_type;
};


struct HOIT_INODE_CACHE{
    UINT                HOITC_ino;
    PHOIT_INODE_CACHE   HOITC_next;
    PHOIT_RAW_INFO      HOITC_nodes;
    UINT                HOITC_nlink;
};


struct HOIT_INODE_INFO{
    mode_t              HOITN_mode;                                     /*  �ļ� mode                   */
    PHOIT_INODE_CACHE   HOITN_inode_cache;
    PHOIT_FULL_DIRENT   HOITN_dents;
    PHOIT_FULL_DNODE    HOITN_metadata;
    PHOIT_FULL_DNODE    HOITN_rbtree;
    PHOIT_VOLUME        HOITN_volume;
    UINT                HOITN_ino;

    uid_t               HOITN_uid;                                      /*  �û� id                     */
    gid_t               HOITN_gid;                                      /*  ��   id                     */
    time_t              RAMN_timeCreate;                                /*  ����ʱ��                    */
    time_t              RAMN_timeAccess;                                /*  ������ʱ��                */
    time_t              RAMN_timeChange;                                /*  ����޸�ʱ��                */
    size_t              RAMN_stSize;                                    /*  ��ǰ�ļ���С (���ܴ��ڻ���) */
    size_t              RAMN_stVSize;                                   /*  lseek ���������С          */
};



struct HOIT_SECTOR{
    PHOIT_SECTOR        HOITS_next;
    UINT                HOITS_bno;                                      /* ���block number              */
    UINT                HOITS_addr;
    UINT                HOITS_length;
    UINT                HOITS_offset;                                   /* ��ǰ��д�����ַ=addr+offset  */
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
