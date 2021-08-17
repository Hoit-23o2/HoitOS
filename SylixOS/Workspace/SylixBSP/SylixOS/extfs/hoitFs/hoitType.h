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
#include "../tools/list/list_interface.h"
#include "../tools/crc/crc32.h"

//!2021-06-06 �޸Ŀ鼶����ṹ���������� by zn
/*********************************************************************************************************
  HoitFs Lib ��ز���
*********************************************************************************************************/
//#define LIB_DEBUG
/*********************************************************************************************************
  HoitFs Cache ��ز���
*********************************************************************************************************/
//#define HOIT_CACHE_TEST
/*********************************************************************************************************
  HoitFs GC ��ز���
*********************************************************************************************************/
// #define GC_DEBUG
// #define GC_TEST
/*********************************************************************************************************
  HoitFs ����� ��ز���
*********************************************************************************************************/
#define RB_TEST
/*********************************************************************************************************
  HoitFs FragTree ��ز���
*********************************************************************************************************/
//#define FT_TEST
//#define FT_DEBUG
#define FT_OBSOLETE_TREE_LIST             

/*********************************************************************************************************
  HoitFs LOG ��ز���
*********************************************************************************************************/
//#define DEBUG_LOG
#define  LOG_TEST
/*********************************************************************************************************
  HoitFs ���Ժ��
*********************************************************************************************************/
#define  MULTI_THREAD_ENABLE      /* ���ö��߳� */
#define  EBS_ENABLE               /* ����EBS */
#define  WRITE_BUFFER_ENABLE      /* ����WriteBuffer */
// #define  BACKGOURND_GC_ENABLE     /* ���ú�̨GC */
// #define  CRC_DATA_ENABLE          /*  CRC DATA���� */
//! 07-18 ZN ��ʱע��log
// #define  LOG_ENABLE

/*********************************************************************************************************
  HoitFs Error Type
*********************************************************************************************************/
#define LOG_APPEND_OK            0
#define LOG_APPEND_NO_SECTOR    -1
#define LOG_APPEND_BAD_ENTITY   -2

/*********************************************************************************************************
  HoitFs�궨��
*********************************************************************************************************/
#define HOIT_MAGIC_NUM                      0x05201314
#define HOIT_FIELD_TYPE                     0xE0000000          //ǰ3λ��ΪTYPE��

#define HOIT_FLAG_TYPE_INODE                0x20000000     //raw_inode���� 0010
#define HOIT_FLAG_TYPE_DIRENT               0x40000000     //raw_dirent����  0100
#define HOIT_FLAG_TYPE_LOG                  0x80000000     //raw_log���� 1000

#define HOIT_FLAG_NOT_OBSOLETE              0x00000001     //Flag�����һλ������ʾ�Ƿ���ڣ�1��û���ڣ�0�ǹ���
#define HOIT_FLAG_OBSOLETE                  0x00000000
#define HOIT_ERROR                          100
#define HOIT_ROOT_DIR_INO                   1   /* HoitFs�ĸ�Ŀ¼��inoΪ1 */
#define HOIT_MAX_DATA_SIZE                  1024
#define __HOIT_IS_OBSOLETE(pRawHeader)      ((pRawHeader->flag & HOIT_FLAG_NOT_OBSOLETE)    == 0)
#define __HOIT_IS_TYPE_INODE(pRawHeader)    ((pRawHeader->flag & HOIT_FLAG_TYPE_INODE)  != 0)
#define __HOIT_IS_TYPE_DIRENT(pRawHeader)   ((pRawHeader->flag & HOIT_FLAG_TYPE_DIRENT) != 0)
#define __HOIT_IS_TYPE_LOG(pRawHeader)      ((pRawHeader->flag & HOIT_FLAG_TYPE_LOG)    != 0)      

#define __HOIT_VOLUME_LOCK(pfs)             API_SemaphoreMPend(pfs->HOITFS_hVolLock, \
                                            LW_OPTION_WAIT_INFINITE)
#define __HOIT_VOLUME_UNLOCK(pfs)           API_SemaphoreMPost(pfs->HOITFS_hVolLock)
#define GET_FREE_LIST(pfs)                  pfs->HOITFS_freeSectorList
#define GET_DIRTY_LIST(pfs)                 pfs->HOITFS_dirtySectorList
#define GET_CLEAN_LIST(pfs)                 pfs->HOITFS_cleanSectorList
#define __HOIT_MIN_4_TIMES(value)           ((value+3)/4*4) /* ��value��չ��4�ı��� */

/*********************************************************************************************************
  �ļ���������
*********************************************************************************************************/
#define __HOITFS_VOL_LOCK(pfs)        API_SemaphoreMPend(pfs->HOITFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __HOITFS_VOL_UNLOCK(pfs)      API_SemaphoreMPost(pfs->HOITFS_hVolLock)

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
typedef struct HOIT_ERASABLE_SECTOR       HOIT_ERASABLE_SECTOR;
typedef struct HOIT_LOG_SECTOR            HOIT_LOG_SECTOR;
typedef struct hoit_rb_node               HOIT_RB_NODE;
typedef struct hoit_rb_tree               HOIT_RB_TREE;
typedef struct hoit_frag_tree             HOIT_FRAG_TREE;
typedef struct hoit_frag_tree_node        HOIT_FRAG_TREE_NODE;
typedef struct hoit_frag_tree_list_node   HOIT_FRAG_TREE_LIST_NODE;
typedef struct hoit_frag_tree_list_header HOIT_FRAG_TREE_LIST_HEADER;

typedef struct HOIT_CACHE_BLK             HOIT_CACHE_BLK;
typedef struct HOIT_CACHE_HDR             HOIT_CACHE_HDR;
typedef struct HOIT_EBS_ENTRY             HOIT_EBS_ENTRY;

typedef struct HOIT_LOG_INFO              HOIT_LOG_INFO;
typedef struct HOIT_RAW_LOG               HOIT_RAW_LOG;

typedef struct hoit_merge_buffer          HOIT_MERGE_BUFFER;
typedef struct hoit_merge_entry           HOIT_MERGE_ENTRY;

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
typedef HOIT_LOG_SECTOR*                  PHOIT_LOG_SECTOR;
typedef HOIT_RB_NODE *                    PHOIT_RB_NODE;
typedef HOIT_RB_TREE *                    PHOIT_RB_TREE;
typedef HOIT_FRAG_TREE *                  PHOIT_FRAG_TREE;
typedef HOIT_FRAG_TREE_NODE *             PHOIT_FRAG_TREE_NODE;
typedef HOIT_FRAG_TREE_LIST_NODE *        PHOIT_FRAG_TREE_LIST_NODE;
typedef HOIT_FRAG_TREE_LIST_HEADER *      PHOIT_FRAG_TREE_LIST_HEADER;


typedef HOIT_CACHE_BLK *                  PHOIT_CACHE_BLK;
typedef HOIT_CACHE_HDR *                  PHOIT_CACHE_HDR;
typedef HOIT_EBS_ENTRY *                  PHOIT_EBS_ENTRY;

typedef HOIT_LOG_INFO *                   PHOIT_LOG_INFO;
typedef HOIT_RAW_LOG *                    PHOIT_RAW_LOG;

typedef HOIT_MERGE_BUFFER *               PHOIT_MERGE_BUFFER;
typedef HOIT_MERGE_ENTRY *                PHOIT_MERGE_ENTRY;
DEV_HDR          HOITFS_devhdrHdr;

DECLARE_LIST_TEMPLATE(PHOIT_ERASABLE_SECTOR);
// USE_LIST_TEMPLATE(hoitType, HOIT_FRAG_TREE_NODE);
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
    
                                                                           /*! GC ��� */
    PHOIT_ERASABLE_SECTOR           HOITFS_erasableSectorList;                  /* �ɲ���Sector�б� */
    List(PHOIT_ERASABLE_SECTOR)     HOITFS_dirtySectorList;                     /* ����obsolete�Ŀ� */ 
    List(PHOIT_ERASABLE_SECTOR)     HOITFS_cleanSectorList;                     /* ����obsolete�Ŀ� */
    List(PHOIT_ERASABLE_SECTOR)     HOITFS_freeSectorList;                      /* ɶ�������Ŀ� */
    Iterator(PHOIT_ERASABLE_SECTOR) HOITFS_sectorIterator;                      /* ͳһsector������ */
    
    PHOIT_ERASABLE_SECTOR   HOITFS_curGCSector;                            /* ��ǰ����GC��Sector */
    LW_OBJECT_HANDLE        HOITFS_GCMsgQ;                                 /* GC�߳���Ϣ����*/
    LW_OBJECT_HANDLE        HOITFS_hGCThreadId;                            /* GC���߳�ID */
    BOOL                    HOITFS_bShouldKillGC;                          /* �Ƿ�ֹͣ��̨GC */
    
    ULONG                   ulGCForegroundTimes;                           /* GCǰ̨���� */
    ULONG                   ulGCBackgroundTimes;                           /* GC��̨���� */
    
    size_t                  HOITFS_totalUsedSize;                          /* hoitfs��ʹ��Flash��С */
    size_t                  HOITFS_totalSize;                              /* ��Flash��С */
    
                                                                           /*! Cache */
    PHOIT_CACHE_HDR         HOITFS_cacheHdr;                               /* hoitfs��cacheͷ�ṹ */

                                                                           /*! Log��� */
    PHOIT_LOG_INFO          HOITFS_logInfo;

} HOIT_VOLUME;


/*********************************************************************************************************
  HoitFs ����ʵ��Ĺ���Header����
*********************************************************************************************************/
struct HOIT_RAW_HEADER{ //32B
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;
    mode_t              file_type;
    UINT                ino;
    UINT                version;
    UINT32              crc;
};


/*********************************************************************************************************
  HoitFs raw inode����
*********************************************************************************************************/
struct HOIT_RAW_INODE { //32B
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;     /* ����ͷ�������ݳ��� */
    mode_t              file_type;
    UINT                ino;
    UINT                version;
    UINT32              crc;
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
    UINT32              crc;
    UINT                pino;
};

struct HOIT_RAW_INFO{ //32B
    UINT                phys_addr;
    UINT                totlen;
    PHOIT_RAW_INFO      next_phys;                                     /* �������ڽӵ���һ�� */
    PHOIT_RAW_INFO      next_logic;                                    /* ͬ��һ��ino����һ�� */
    UINT                is_obsolete;
};                                     

struct HOIT_FULL_DNODE{//32B
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
    PHOIT_MERGE_BUFFER  HOITN_pMergeBuffer;

    uid_t               HOITN_uid;                                      /*  �û� id                     */
    gid_t               HOITN_gid;                                      /*  ��   id                     */
    time_t              HOITN_timeCreate;                               /*  ����ʱ��                    */
    time_t              HOITN_timeAccess;                               /*  ������ʱ��                */
    time_t              HOITN_timeChange;                               /*  ����޸�ʱ��                */
    size_t              HOITN_stSize;                                   /*  ��ǰ�ļ���С (���ܴ��ڻ���) */
    size_t              HOITN_stVSize;                                  /*  lseek ���������С          */
};



struct HOIT_ERASABLE_SECTOR{  //100B
    PHOIT_ERASABLE_SECTOR         HOITS_next;                                     /* ������Ϣ���� */
    UINT                          HOITS_bno;                                      /* Setcor��block number              */
    UINT                          HOITS_addr;
    UINT                          HOITS_length;
    UINT                          HOITS_offset;                                   /* ��ǰ��д�����ַ = addr+offset  */
    UINT                          HOITS_uiObsoleteEntityCount;                    /* sector��ǰ��������ʵ������ */
    UINT                          HOITS_uiAvailableEntityCount;                   /* sector��ǰ������Чʵ������ */
    //TODO: д�롢GCʱ����Ҫ���и���
    spinlock_t                    HOITS_lock;                                     /* ÿ��Sector����һ��? */ 

    UINT                          HOITS_uiUsedSize;
    UINT                          HOITS_uiFreeSize;
    
    ////UINT                      HOITS_uiNTotalRawInfo;                          /* �ÿɲ���Sector��RawInfo������ */
    PHOIT_RAW_INFO                HOITS_pRawInfoFirst;                            /* ָ��ɲ���Sector�е�һ������ʵ�� */
    PHOIT_RAW_INFO                HOITS_pRawInfoLast;                             /* ָ��ɲ���Sector�����һ������ʵ�壬ͨ��next_phys��ȡ��һ������ʵ�� */
    
    PHOIT_RAW_INFO                HOITS_pRawInfoCurGC;                            /* ��ǰ�������յ�����ʵ�壬ע��һ�ν�����һ������ʵ�� */
    PHOIT_RAW_INFO                HOITS_pRawInfoPrevGC;
    PHOIT_RAW_INFO                HOITS_pRawInfoLastGC;                           /* ���һ��Ӧ����GC��Raw Info */
  
    ULONG                         HOITS_tBornAge;                                 /* ��ǰSector�ĳ���ʱ�� */                        
};

struct HOIT_LOG_SECTOR{
    PHOIT_LOG_SECTOR        pErasableNextLogSector;
    
    PHOIT_ERASABLE_SECTOR   pErasableSetcor;    
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

    UINT32 uiMemoryBytes;                                        /* add by PYQ ���ڹ۲��С */
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
    PHOIT_MERGE_ENTRY pMergeEntry;
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
  FragTree Visitor�ṹ
*********************************************************************************************************/
typedef enum vis_statue
{
    VIS_CONTINUE,
    VIS_END,
    VIS_ERROR
} VIS_STATUE;

typedef VIS_STATUE (*visitorHoitFragTree)(PHOIT_FRAG_TREE pFTTree, PHOIT_FRAG_TREE_NODE pFTn, PVOID pUserValue, PVOID *ppReturn);
/*********************************************************************************************************
  HOITFS cache�ṹ
*********************************************************************************************************/
typedef struct HOIT_CACHE_BLK
{
    BOOL                        HOITBLK_bType;
    PHOIT_ERASABLE_SECTOR       HOITBLK_sector;           /* cacheӳ���Sector�����ڻ�ȡ��Ϣ */
    UINT32                      HOITBLK_blkNo;            /* cache��� */
    struct HOIT_CACHE_BLK       *HOITBLK_cacheListPrev;   /* ��������һ��cache */
    struct HOIT_CACHE_BLK       *HOITBLK_cacheListNext;   /* ��������һ��cache */
    PCHAR                       HOITBLK_buf;              /* ����?? */
    //! 2021-08-14 PYQ �۲�
    UINT32                      HOITBLK_uiCurOfs;
}HOIT_CACHE_BLK;

/*********************************************************************************************************
  HOITFS cache ����ͷ�ṹ
*********************************************************************************************************/
typedef struct HOIT_CACHE_HDR
{
    PHOIT_VOLUME            HOITCACHE_hoitfsVol;
    size_t                  HOITCACHE_blockSize;    /* ����cache��С */
    UINT32                  HOITCACHE_blockMaxNums; /* cache������� */
    UINT32                  HOITCACHE_blockNums;    /* ��ǰcache���� */
    LW_OBJECT_HANDLE        HOITCACHE_hLock;        /* cache������? */
    UINT32                  HOITCACHE_flashBlkNum;  /* ��flash�ֿ��Ŀ��� */
    UINT32                  HOITCACHE_nextBlkToWrite;/* ��һ��Ҫ����Ŀ� */
    PHOIT_CACHE_BLK         HOITCACHE_cacheLineHdr;  /* cache����ͷ��ע��ýڵ㲻�������� */
    
    //! 2021-07-04 ZN filter��
    // size_t                  HOITCACHE_EBSEntrySize; /* EBS enty��С */
    size_t                  HOITCACHE_CRCMagicAddr;   /* EBS ���� CRC У����λ�� */
    size_t                  HOITCACHE_EBSStartAddr;   /* EBS ��sector����ʼ��ַ */
    size_t                  HOITCACHE_PageAmount;     /* ����cache sector�е�ҳ������Ҳ��EBS entry��������(������1k -1) */

} HOIT_CACHE_HDR;

//! 2021-7-04 ZN EBS��
/*********************************************************************************************************
  HOITFS EBS entry
*********************************************************************************************************/
typedef struct HOIT_EBS_ENTRY
{
    UINT32  HOIT_EBS_ENTRY_inodeNo;     /* �����ļ�inode�ţ�δʹ��ʱȫ1 */
    UINT16  HOIT_EBS_ENTRY_obsolete;    /* ���ڱ�־��δ����ʱȫ1������ʱȫ0 */
    UINT16  HOIT_EBS_ENTRY_pageNo;      /* ����ʵ����sector�ϵ��׸�ҳ��ҳ�� */
}HOIT_EBS_ENTRY;

/*********************************************************************************************************
  HOITFS log �ļ�ͷ
*********************************************************************************************************/
struct HOIT_LOG_INFO
{
    PHOIT_LOG_SECTOR pLogSectorList;              /* LOG Sector�������  */
    UINT             uiRawLogHdrAddr;             /* ���ļ�ͷָ����� LOG��ַ */
    UINT             uiLogCurAddr;
    UINT             uiLogCurOfs;
    UINT             uiLogSize;
    UINT             uiLogEntityCnt; 
};

struct HOIT_RAW_LOG
{
    UINT32              magic_num;
    UINT32              flag;
    UINT32              totlen;
    mode_t              file_type;
    UINT                ino;
    UINT                version;

    UINT                uiLogSize;
    UINT                uiLogFirstAddr;
};
/*********************************************************************************************************
  HOITFS MergeBuffer �ṹ
*********************************************************************************************************/
struct hoit_merge_buffer
{
    UINT32                  size;
    UINT32                  threshold;  //�����ϲ������Ľڵ���
    PHOIT_MERGE_ENTRY       pList;
};
struct hoit_merge_entry
{
    PHOIT_FRAG_TREE_NODE    pTreeNode;
    PHOIT_MERGE_ENTRY       pNext;
    PHOIT_MERGE_ENTRY       pPrev;
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

static inline PVOID hoit_malloc(PHOIT_VOLUME pfs, size_t stNBytes){
    pfs->HOITFS_ulCurBlk += stNBytes;
    if(pfs->HOITFS_ulCurBlk > pfs->HOITFS_ulMaxBlk){
       pfs->HOITFS_ulMaxBlk = pfs->HOITFS_ulCurBlk;
    }
    return lib_malloc(stNBytes);
}

static inline PVOID hoit_free(PHOIT_VOLUME pfs, PVOID pvPtr, size_t stNBytes){
    pfs->HOITFS_ulCurBlk -= stNBytes;
    lib_free(pvPtr);
}
/*********************************************************************************************************
  Common ���÷�����
*********************************************************************************************************/
BOOL                  hoitLogCheckIfLog(PHOIT_VOLUME pfs, PHOIT_ERASABLE_SECTOR pErasableSector);
PHOIT_ERASABLE_SECTOR hoitFindAvailableSector(PHOIT_VOLUME pfs);

#ifdef CRC_DATA_ENABLE
static UINT32         hoit_crc32_le(PUCHAR p, UINT len){
    return crc32_le(p, len);
}
#else 
static UINT32         hoit_crc32_le(PUCHAR p, UINT len){

}
#endif  /* CRC_DATA_ENABLE */
#endif /* SYLIXOS_EXTFS_HOITFS_HOITTYPE_H_ */
