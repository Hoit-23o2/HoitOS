#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
//#include "../SylixOS/kernel/include/k_internal.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
//#include "../SylixOS/system/include/s_class.h"
//#include "../SylixOS/kernel/include/k_const.h"
#include "../driver/mtd/nor/fake_nor.h"
#include "../driver/mtd/nor/fake_nor_cmd.h"

/*********************************************************************************************************
  ���·���ִ��Ƿ�Ϊ��Ŀ¼����ֱ��ָ���豸
*********************************************************************************************************/

#define __STR_IS_ROOT(pcName)           ((pcName[0] == PX_EOS) || (lib_strcmp(PX_STR_ROOT, pcName) == 0))

/*********************************************************************************************************
  ����궨��
*********************************************************************************************************/
/* global sector_no */
#define GET_NOR_OFFSET_FROM_SECN(sector_no)                 (sector_no*NOR_FLASH_SECTORSZ)
#define GET_NOR_OFFSET_FROM_BLKN(block_no)                  (block_no*NOR_FLASH_BLKSZ)
/* local sector_no */
#define GET_OFFSET_FROM_BLK_AND_SECN(block_no,sector_no)    (block_no*NOR_FLASH_BLKSZ+sector_no*NOR_FLASH_SECTORSZ)
#define GET_BLKN_OF_SECN(sector_no)                         (sector_no/NOR_FLASH_NBLK)
#define GET_SEC_IDX_IN_BLK(sector_no)                       (sector_no%NOR_FLASH_NBLK)
/* ÿ��sector�ɰ������ٸ�inode */
#define INODE_PSEC                                          (NOR_FLASH_SECTORSZ/sizeof(LFS_INODE))

#define ASSIGNABLE_BLOCKS               13              /* 16 - 1(CPR) - 2(FileMap) */
#define MAX_FILE                        1023            /* ����ļ����� */
#define MAX_DATA_SECTOR                 32              /* �����ļ�������ռ���sector���� */
#define CPR_FN_1_BLK                    0               /* ��һ��CheckpointRegion_FileNameMap */
#define CPR_FN_2_BLK                    1               /* �ڶ���CheckpointRegion_FileNameMap */

/********************************     Summary sector     **********************************************/
#define SUMMARY_INODE_TYPE              1024
#define SUMMARY_IMAP_TYPE               1025            /* summary �� secotr ��¼ imap ��־ */
#define SUMMARY_SUMMARY_TYPE            1026            /* summary �� secotr ��¼ summary sector ��־ */

/********************************     FileMap               ******************************************/
#define FN_ENTRY_PBLK                   512             /* ÿ��Block��FileNameEntry�ĸ��� */
#define FN_ENTRY_SZ                     128             /* ÿ��FileNameEntry�Ĵ�СΪ128B */
#define MAX_FN_SZ                       126             /* һ���ļ������Ϊ126B */

#define BLK_CLEAN                       0               /* CPR.free_sec�иɾ���־ */
#define BLK_DIRTY                       1               /* CPR.free_sec�����־ */

//#define UINT_MAX                        4294967295      /* inode,block,secotr������Ҳ���ʱ�ķ���ֵ */

/*********************************************************************************************************
  ����
*********************************************************************************************************/
typedef struct 
{
    UINT                imap_sec;                 /* imap ����sector�����    */
    UINT                summary_sec;              /* summary ����sector����� */
    UINT8               free_sec[NOR_FLASH_NBLK]; /* ��ǰblock����sector      */              
}CHECKPOINT_REGION;
typedef CHECKPOINT_REGION *PCHECKPOINT_REGION;

typedef struct 
{
    LW_DEV_HDR          LFS_devhdrHdr;                            /* lfs �豸ͷ   */
    LW_OBJECT_HANDLE    LFS_hVolLock;                             /* �豸��       */ 
    LW_LIST_LINE_HEADER LFS_plineFdNodeHeader;                  /*  fd_node ����                */
    /* LFSȫ�ֱ��� */
    UINT                LFS_blockSummary[NOR_FLASH_SECTPBLK \
                                            *NOR_FLASH_NBLK][2];  /* ����block����һ��summary secotr����һ��Ϊinode�ţ��ڶ���Ϊ��Чλ */
    CHAR                LFS_secBuf[NOR_FLASH_SECTORSZ];           /* ��sectorΪ��λ�Ļ����� */
    CHAR                LFS_blkBuf[NOR_FLASH_BLKSZ];              /* Block Buffer */
    
    BOOL                LFS_bValid;                               /* �Ƿ���Ч���Ƿ�û��ж�أ� */
    BOOL                LFS_bForceDelete;                         /* �Ƿ�����ǿ��ɾ�� */
    PCHECKPOINT_REGION  LFS_cpr;                                  /* CPR�ṹ��ָ�� */
    UINT                LFS_imap[MAX_FILE+1];                     /* IMAPҲ���ڴ���ά�� */
    UINT                LFS_curBlockNo;                           /* ��ǰBlock����Χ(2~31)��0��1��ռ�� */
    UINT                LFS_availableSector;                      /* �ڵ�ǰBLOCK_NO�У���һ������sector����Χ(0~15) */
    UINT8               LFS_cleanBlock[NOR_FLASH_NBLK];           /* �鿴�Ŀ�BLOCK�Ƿ�����ģ�0Ϊ�ɾ���1Ϊ�� */
    UINT8               LFS_inodeMap[MAX_FILE];                   /* inodeռ����� */

    INT                 LFS_availableFile;                        /* �����ļ����� */
    UINT                LFS_ulCurSector;                         /* �ļ�ϵͳ��ǰռ��sector */
   UINT                 LFS_ulMaxSector;                          /* ����ļ�ռ��sector���� */

    mode_t              LFS_mode;                 /* �ļ�mode     */
    time_t              LFS_time;                 /* ����ʱ��     */  
    uid_t               LFS_uid;                  /* �û�id       */
    gid_t               LFS_gid;                  /* ��id         */
}LFS_VOLUME;
typedef LFS_VOLUME *PLFS_VOLUME;



typedef struct
{                                     
  UINT                  LFSN_inodeNo;
  PCHAR                 LFSN_pcname;                                    /* �ļ����� */

  mode_t                LFSN_mode; 
  uid_t                 LFSN_uid;                                       /*  �û� id                     */
  gid_t                 LFSN_gid;  

  INT                   LFSN_bChange;                                   /*  �Ƿ��޸�                  */
  time_t                LFSN_timeCreate;                                /*  ����ʱ��                    */
  time_t                LFSN_timeAccess;                                /*  ������ʱ��                */
  time_t                LFSN_timeChange;                                /*  ����޸�ʱ��                */

  size_t                LFSN_stSize;                                /* �ļ���С�����ֽ�Ϊ��λ�����Ϊ32��sector����128KB */
  size_t                LFSN_stVSize;                               /* lseek ���������С */
  UINT                  LFSN_Cnt;                                   /* �ļ�ռ��sector��С */

  UINT                  LFSN_sectorLocation[MAX_DATA_SECTOR];       /* �������ݵ�sector��ȫ��λ�� */
  UINT                  LFSN_other[16];                             /* ��������Ϊ�˶���128KB */
}LFS_INODE;
typedef LFS_INODE *PLFS_INODE;


/*********************************************************************************************************
  �����嵥
*********************************************************************************************************/
VOID initBlocks();                                          /* ��ʼ��ÿ��block���Լ�summary sector */
VOID initCPR_FNMap();                                       /* ��ʼ��CPR_FNMap���򣬼�CheckpointRegion_FileNameMap�� */
VOID initCleanBlock();                                      /* ��ʼ��CleanBlock */
INT initLFS(PLFS_VOLUME plfs);                              /* ��ʼ��LFS�ĸ��ֻ����� */

/*  �м����к���������Ҫ�޸�    */
INT readInCheckpointRegion(PLFS_VOLUME plfs);               /* ��ȡCPR���ڴ� yes*/
INT findNextAvailableSector();                              /* ����һ������Block���ѽ��������ȫ�ֱ���BLOCK_NO��AVAILABLE_SECTOR�� */
INT readSummary();                                          /* ��ȡ��ǰsummary sector��BLOCK_SUMMARY�� */
INT readInSector(PLFS_VOLUME plfs, UINT sector_no);         /* ��ȡ��ǰsector��SEC_BUF�� */
INT readInBlock(PLFS_VOLUME plfs, UINT blk_no);             /* ����ǰblock��BLK_BUF�� */
UINT readInImapSector(UINT address, UINT fragment_no);      /* ��imap sector��IMAP�� */

INT updateCheckpointRegion(PLFS_VOLUME plfs);                            /* ��дCheckoutPoint Region  */
UINT findNextCleanBlock(PLFS_VOLUME plfs);                  /* Ѱ����һ�����е�Segment����CPR�в��� */
UINT writeOutSector(PLFS_VOLUME plfs);                      /* ����ǰSEC_BUF����д��LFS_curBlockNo��sector�� */
VOID writeOutBlock(PLFS_VOLUME plfs, UINT block_no);        /* ����ǰBLK_BUF����д��LFS_curBlockNoָ����block�� */
UINT nextInodeNumber(PLFS_VOLUME plfs);                     /* Ѱ����һ��Inode Number��ͨ��FileMap������ */
INT  writeInode(PLFS_VOLUME plfs, 
                  const PLFS_INODE pnode, 
                  UINT inode_nor);                          /* ׷��дInode�����п� */
INT updateFilemap(PLFS_VOLUME plfs,
                    UINT inode_number,
                    PCHAR lfs_filename);                    /* ����Filemap */
INT updateImap(PLFS_VOLUME plfs, 
                UINT inode_number, 
                UINT sector_position);                      /* ����imap */

INT updateSummary(PLFS_VOLUME plfs);

PLFS_INODE getInodeFromInumber(PLFS_VOLUME plfs,
                                UINT inode_number);         /* ��ȡinode������inodeָ�롣���ʧ�ܣ�����Null */
UINT getInodeNumberOfFile(PLFS_VOLUME plfs,
                          PCHAR pcname);                    /* �����ļ�����ȡinode��� */
INT getFileNameOfInodeNumber( PLFS_VOLUME plfs,
                              PCHAR pcname,
                              UINT inode_number);
VOID printSector(UINT sector_no);                           /* ���Sector */

/* lFs.c�������� */
PLFS_INODE __lfs_open(  PLFS_VOLUME     plfs,
                        PCHAR           pcName,
                        BOOL            *pbRoot);
/* lfs ����һ���ļ� */
PLFS_INODE  __lfs_maken( PLFS_VOLUME     plfs,
                        PCHAR           pcname,
                        mode_t          mode);
/* lfs ��ȡ�ļ�״̬ */                        
VOID        __lfs_stat( PLFS_INODE plfsn, 
                        PLFS_VOLUME plfs, 
                        struct stat  *pstat);
/* LFS ɾ��һ���ļ� */
VOID        __lfs_close(PLFS_INODE plfsn, INT iFlag);

/* LFSɾ��һ���ļ� */
INT         __lfs_unlink(PLFS_VOLUME plfs, PLFS_INODE plfsn);

/* LFSж���ļ�ϵͳ */
VOID        __lfs_unmount(PLFS_VOLUME plfs);

/* LFS д���ļ����� */
ssize_t     __lfs_write (  PLFS_VOLUME plfs,
                          PLFS_INODE  plfsn, 
                          CPVOID  pvBuffer, 
                          size_t  stNBytes, 
                          size_t  stOft);

ssize_t     __lfs_read( PLFS_VOLUME plfs,
                        PLFS_INODE plfsn,
                        PVOID pvBuffer,
                        size_t stSize,
                        size_t stOft);
INT __lfs_move (PLFS_VOLUME plfs, 
                PLFS_INODE plfsn, 
                PCHAR pcNewName);
