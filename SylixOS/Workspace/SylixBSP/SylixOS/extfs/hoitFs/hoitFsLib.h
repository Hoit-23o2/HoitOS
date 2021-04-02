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

#ifndef __HOITFSLIB_H
#define __HOITFSLIB_H

#include "SylixOS.h"                                                    /*  ����ϵͳ                    */

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
#define HOIT_ERROR          500

#define __HOIT_SB_LOCK(pfs)        API_SemaphoreMPend(pfs->HOITFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __HOIT_SB_UNLOCK(pfs)      API_SemaphoreMPost(pfs->HOITFS_hVolLock)

/*********************************************************************************************************
  ���·���ִ��Ƿ�Ϊ��Ŀ¼����ֱ��ָ���豸
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)           ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))

/*********************************************************************************************************
  HoitFs super block����
*********************************************************************************************************/
typedef struct {
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
    PHOIT_BLOCK         HOITFS_now_block;
    UINT                HOITFS_highest_ino;
} HOIT_VOLUME;
typedef HOIT_VOLUME* PHOIT_VOLUME;

/*********************************************************************************************************
  HoitFs ����ʵ��Ĺ���Header����
*********************************************************************************************************/
typedef struct {
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;
    mode_t              file_type;
} HOIT_RAW_HEADER;
typedef HOIT_RAW_HEADER* PHOIT_RAW_HEADER;
/*********************************************************************************************************
  HoitFs raw inode����
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
  HoitFs raw dirent����
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
    UINT                HOITFD_ino;                                      /*  �ļ�inode number            */
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
    mode_t              HOITN_mode;                                      /*  �ļ� mode                   */
    PHOIT_INODE_CACHE   HOITN_inode_cache;
    PHOIT_FULL_DIRENT   HOITN_dents;
    PHOIT_FULL_DNODE    HOITN_metadata;
    PHOIT_FULL_DNODE    HOITN_rbtree;
    PHOIT_VOLUME        HOITN_volume;
    UINT                HOITN_ino;

    uid_t               HOITN_uid;                                       /*  �û� id                     */
    gid_t               HOITN_gid;                                       /*  ��   id                     */
    time_t              RAMN_timeCreate;                                /*  ����ʱ��                    */
    time_t              RAMN_timeAccess;                                /*  ������ʱ��                */
    time_t              RAMN_timeChange;                                /*  ����޸�ʱ��                */
    size_t              RAMN_stSize;                                    /*  ��ǰ�ļ���С (���ܴ��ڻ���) */
    size_t              RAMN_stVSize;                                   /*  lseek ���������С          */
}HOIT_INODE_INFO;
typedef HOIT_INODE_INFO* PHOIT_INODE_INFO;


typedef struct {
    UINT HOITB_addr;
    UINT HOITB_length;
    UINT HOITB_offset;                                                  /* ��ǰ��д�����ַ=addr+offset  */
} HOIT_BLOCK;
typedef HOIT_BLOCK* PHOIT_BLOCK;

#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0       */
#endif                                                                  /*  __HOITFSLIB_H                */
