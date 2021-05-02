/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: hoitType.h
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 03 �� 19 ��
**
** ��        ��: ȫ�������ļ�����ֹ����ѭ��
*********************************************************************************************************/
#ifndef SYLIXOS_EXTFS_HOITFS_HOITTYPE_H_
#define SYLIXOS_EXTFS_HOITFS_HOITTYPE_H_
#define  __SYLIXOS_KERNEL
#include "stdio.h"
#include "SylixOS.h"

/*********************************************************************************************************
  HoitFs�궨��
*********************************************************************************************************/
#define HOIT_MAGIC_NUM                      0x05201314
#define HOIT_FIELD_TYPE                     0xE0000000          //ǰ3λ��ΪTYPE��

#define HOIT_FLAG_TYPE_INODE                0x20000000     //raw_inode���� 001
#define HOIT_FLAG_TYPE_DIRENT               0x40000000     //raw_dirent����  010
#define HOIT_FLAG_OBSOLETE                  0x00000001     //Flag�����һλ������ʾ�Ƿ���ڣ�1��û���ڣ�0�ǹ���
#define HOIT_ERROR                          100
#define HOIT_ROOT_DIR_INO                   1   /* HoitFs�ĸ�Ŀ¼��inoΪ1 */
#define __HOIT_IS_OBSOLETE(pRawHeader)      ((pRawHeader->flag & HOIT_FLAG_OBSOLETE)    == 0)
#define __HOIT_IS_TYPE_INODE(pRawHeader)    ((pRawHeader->flag & HOIT_FLAG_TYPE_INODE)  != 0)
#define __HOIT_IS_TYPE_DIRENT(pRawHeader)   ((pRawHeader->flag & HOIT_FLAG_TYPE_DIRENT) != 0)

#define __HOIT_VOLUME_LOCK(pfs)             API_SemaphoreMPend(pfs->HOITFS_hVolLock, \
                                            LW_OPTION_WAIT_INFINITE)
#define __HOIT_VOLUME_UNLOCK(pfs)           API_SemaphoreMPost(pfs->HOITFS_hVolLock)
#define __HOIT_MIN_4_TIMES(value)           ((value+3)/4*4) /* ��value��չ��4�ı��� */

/*********************************************************************************************************
  ���·���ִ��Ƿ�Ϊ��Ŀ¼����ֱ��ָ���豸
*********************************************************************************************************/
#define __STR_IS_ROOT(pcName)               ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))


/*********************************************************************************************************
  C���Խṹ����ǰ����
*********************************************************************************************************/
typedef struct HOIT_VOLUME                HOIT_VOLUME;
typedef struct HOIT_RAW_HEADER            HOIT_RAW_HEADER;
typedef struct HOIT_RAW_INODE             HOIT_RAW_INODE;
typedef struct HOIT_RAW_DIRENT            HOIT_RAW_DIRENT;
typedef struct HOIT_RAW_INFO              HOIT_RAW_INFO;
typedef struct HOIT_FULL_DNODE            HOIT_FULL_DNODE;
typedef struct HOIT_FULL_DIRENT           HOIT_FULL_DIRENT;
typedef struct HOIT_INODE_CACHE           HOIT_INODE_CACHE;
typedef struct HOIT_INODE_INFO            HOIT_INODE_INFO;
typedef struct HOIT_ERASABLE_SECTOR                HOIT_ERASABLE_SECTOR;
typedef struct hoit_rb_node               HOIT_RB_NODE;
typedef struct hoit_rb_tree               HOIT_RB_TREE;
typedef struct hoit_frag_tree             HOIT_FRAG_TREE;
typedef struct hoit_frag_tree_node        HOIT_FRAG_TREE_NODE;
typedef struct hoit_frag_tree_list_node   HOIT_FRAG_TREE_LIST_NODE;
typedef struct hoit_frag_tree_list_header HOIT_FRAG_TREE_LIST_HEADER;

typedef HOIT_VOLUME*                      PHOIT_VOLUME;
typedef HOIT_RAW_HEADER*                  PHOIT_RAW_HEADER;
typedef HOIT_RAW_INODE*                   PHOIT_RAW_INODE;
typedef HOIT_RAW_DIRENT*                  PHOIT_RAW_DIRENT;
typedef HOIT_RAW_INFO*                    PHOIT_RAW_INFO;
typedef HOIT_FULL_DNODE*                  PHOIT_FULL_DNODE;
typedef HOIT_FULL_DIRENT*                 PHOIT_FULL_DIRENT;
typedef HOIT_INODE_CACHE*                 PHOIT_INODE_CACHE;
typedef HOIT_INODE_INFO*                  PHOIT_INODE_INFO;
typedef HOIT_ERASABLE_SECTOR*             PHOIT_ERASABLE_SECTOR;
typedef HOIT_RB_NODE *                    PHOIT_RB_NODE;
typedef HOIT_RB_TREE *                    PHOIT_RB_TREE;
typedef HOIT_FRAG_TREE *                  PHOIT_FRAG_TREE;
typedef HOIT_FRAG_TREE_NODE *             PHOIT_FRAG_TREE_NODE;
typedef HOIT_FRAG_TREE_LIST_NODE *        PHOIT_FRAG_TREE_LIST_NODE;
typedef HOIT_FRAG_TREE_LIST_HEADER *      PHOIT_FRAG_TREE_LIST_HEADER;

DEV_HDR          HOITFS_devhdrHdr;



/*********************************************************************************************************
  HoitFs super block����
*********************************************************************************************************/
typedef struct HOIT_VOLUME{
    DEV_HDR                 HOITFS_devhdrHdr;                                /*  HoitFs �ļ�ϵͳ�豸ͷ        */
    LW_OBJECT_HANDLE        HOITFS_hVolLock;                                 /*  �������                    */
    LW_LIST_LINE_HEADER     HOITFS_plineFdNodeHeader;                        /*  fd_node ����                */
    PHOIT_INODE_INFO        HOITFS_pRootDir;                                 /*  ��Ŀ¼�ļ��ݶ�Ϊһֱ�򿪵�  */
    PHOIT_FULL_DIRENT       HOITFS_pTempRootDirent;

    BOOL                    HOITFS_bForceDelete;                             /*  �Ƿ�����ǿ��ж�ؾ�          */
    BOOL                    HOITFS_bValid;

    uid_t                   HOITFS_uid;                                      /*  �û� id                     */
    gid_t                   HOITFS_gid;                                      /*  ��   id                     */
    mode_t                  HOITFS_mode;                                     /*  �ļ� mode                   */
    time_t                  HOITFS_time;                                     /*  ����ʱ��                    */
    ULONG                   HOITFS_ulCurBlk;                                 /*  ��ǰ�����ڴ��С            */
    ULONG                   HOITFS_ulMaxBlk;                                 /*  ����ڴ�������              */

    PHOIT_INODE_CACHE       HOITFS_cache_list;
    UINT                    HOITFS_highest_ino;
    UINT                    HOITFS_highest_version;

    PHOIT_ERASABLE_SECTOR   HOITFS_now_sector;
    PHOIT_ERASABLE_SECTOR   HOITFS_erasableSectorList;                     /* �ɲ���Sector�б� */
    PHOIT_ERASABLE_SECTOR   HOITFS_curGCSector;                            /* ��ǰ����GC��Sector */
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
    UINT                version;
};


/*********************************************************************************************************
  HoitFs raw inode����
*********************************************************************************************************/
struct HOIT_RAW_INODE{
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;     /* ����ͷ�������ݳ��� */
    mode_t              file_type;
    UINT                ino;
    UINT                version;
    UINT                offset;     /* ��¼�ļ��ڲ�ƫ�� */
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
    UINT                version;
    UINT                pino;
};

struct HOIT_RAW_INFO{
    UINT                phys_addr;
    UINT                totlen;
    PHOIT_RAW_INFO      next_phys;
    PHOIT_RAW_INFO      next_logic;
    UINT                is_obsolete;
};


struct HOIT_FULL_DNODE{
    PHOIT_FULL_DNODE    HOITFD_next;
    PHOIT_RAW_INFO      HOITFD_raw_info;
    UINT                HOITFD_offset;                                  /*���ļ����ƫ����*/
    UINT                HOITFD_length;                                  /*��Ч�����ݳ���*/
    mode_t              HOITFD_file_type;                               /*�ļ�������*/
    UINT                HOITFD_version;
};


struct HOIT_FULL_DIRENT{
    PHOIT_FULL_DIRENT   HOITFD_next;
    PHOIT_RAW_INFO      HOITFD_raw_info;
    UINT                HOITFD_nhash;
    PCHAR               HOITFD_file_name;
    UINT                HOITFD_ino;                                     /* Ŀ¼��ָ����ļ�inode number      */
    UINT                HOITFD_pino;                                    /* ��Ŀ¼��������Ŀ¼�ļ�inode number*/
    mode_t              HOITFD_file_type;
    UINT                HOITFD_version;
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
    PHOIT_FRAG_TREE     HOITN_rbtree;
    PHOIT_VOLUME        HOITN_volume;
    UINT                HOITN_ino;                                      /*  �涨��Ŀ¼��inoΪ1          */
    PCHAR               HOITN_pcLink;

    uid_t               HOITN_uid;                                      /*  �û� id                     */
    gid_t               HOITN_gid;                                      /*  ��   id                     */
    time_t              HOITN_timeCreate;                                /*  ����ʱ��                    */
    time_t              HOITN_timeAccess;                                /*  ������ʱ��                */
    time_t              HOITN_timeChange;                                /*  ����޸�ʱ��                */
    size_t              HOITN_stSize;                                    /*  ��ǰ�ļ���С (���ܴ��ڻ���) */
    size_t              HOITN_stVSize;                                   /*  lseek ���������С          */
};



struct HOIT_ERASABLE_SECTOR{
    PHOIT_ERASABLE_SECTOR         HOITS_next;
    UINT                          HOITS_bno;                                      /* ���block number              */
    UINT                          HOITS_addr;
    UINT                          HOITS_length;
    UINT                          HOITS_offset;                                   /* ��ǰ��д�����ַ = addr+offset  */

    UINT                          HOITS_uiUsedSize;
    UINT                          HOITS_uiFreeSize;
    
    UINT                          HOITS_uiNTotalRawInfo;                          /* �ÿɲ���Sector��RawInfo������ */
    PHOIT_RAW_INFO                HOITS_pRawInfoFirst;                            /* ָ��ɲ���Sector�е�һ������ʵ�� */
    PHOIT_RAW_INFO                HOITS_pRawInfoLast;                             /* ָ��ɲ���Sector�����һ������ʵ�壬ͨ��next_phys��ȡ��һ������ʵ�� */
    PHOIT_RAW_INFO                HOITS_pRawInfoCurGC;                            /* ��ǰ�������յ�����ʵ�壬ע��һ�ν�����һ������ʵ�� */

};


/*********************************************************************************************************
  ������ڵ�
*********************************************************************************************************/
struct hoit_rb_node
{
    UINT32 uiColor;
    INT32  iKey;
    struct hoit_rb_node* pRbnLeft;
    struct hoit_rb_node* pRbnRight;
    struct hoit_rb_node* pRbnParent;
};



/*********************************************************************************************************
  ������ṹ
*********************************************************************************************************/
struct hoit_rb_tree
{
    PHOIT_RB_NODE pRbnGuard;            /* 哨兵 */
    PHOIT_RB_NODE pRbnRoot;             /* 根节�? */
};



/*********************************************************************************************************
  INODE_INFOָ���FragTree
*********************************************************************************************************/
struct hoit_frag_tree
{
    PHOIT_RB_TREE pRbTree;
    UINT32 uiNCnt;                                               /* �ú�����ڵ���Ŀ */
    PHOIT_VOLUME pfs;
};


/*********************************************************************************************************
  FragTree�ڵ�
*********************************************************************************************************/
struct hoit_frag_tree_node
{
    HOIT_RB_NODE pRbn;
    PHOIT_FULL_DNODE pFDnode;
    UINT32 uiSize;
    UINT32 uiOfs;
};

/*********************************************************************************************************
  PHOIT_FRAG_TREE_LIST_NODE 
*********************************************************************************************************/
struct hoit_frag_tree_list_node
{
    PHOIT_FRAG_TREE_NODE pFTn;
    struct hoit_frag_tree_list_node * pFTlistNext;
};


/*********************************************************************************************************
  PHOIT_FRAG_TREE_LIST_NODE_HEADER
*********************************************************************************************************/
struct hoit_frag_tree_list_header
{
    PHOIT_FRAG_TREE_LIST_NODE pFTlistHeader;
    UINT32 uiLowBound;
    UINT32 uiHighBound;
    UINT32 uiNCnt;
};

/*********************************************************************************************************
  ƫ��������
*********************************************************************************************************/
#define OFFSETOF(type, member)                                      \
        ((size_t)&((type *)0)->member)                              \
/*********************************************************************************************************
  �õ�ptr�������ṹ
*********************************************************************************************************/
#define CONTAINER_OF(ptr, type, member)                             \
        ((type *)((size_t)ptr - OFFSETOF(type, member)))      \

#define HOIT_RAW_DATA_MAX_SIZE      4096    /* ��λΪ Byte */

#endif /* SYLIXOS_EXTFS_HOITFS_HOITTYPE_H_ */
