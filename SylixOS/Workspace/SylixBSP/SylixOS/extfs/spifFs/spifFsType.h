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
** ��   ��   ��: spifFsType.h
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 06 �� 01��
**
** ��        ��: Spiffs�ļ�ϵͳ����
*********************************************************************************************************/

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL

#ifndef SYLIXOS_EXTFS_SPIFFS_SPIFFSTYPE_H_
#define SYLIXOS_EXTFS_SPIFFS_SPIFFSTYPE_H_
#include "spifFsConfig.h"
/*********************************************************************************************************
 * ��غ궨��
*********************************************************************************************************/
#define SPIFFS_OK                       0
#define SPIFFS_ERR_NOT_MOUNTED          -10000
#define SPIFFS_ERR_FULL                 -10001
#define SPIFFS_ERR_NOT_FOUND            -10002
#define SPIFFS_ERR_END_OF_OBJECT        -10003
#define SPIFFS_ERR_DELETED              -10004
#define SPIFFS_ERR_NOT_FINALIZED        -10005
#define SPIFFS_ERR_NOT_INDEX            -10006
#define SPIFFS_ERR_OUT_OF_FILE_DESCS    -10007
#define SPIFFS_ERR_FILE_CLOSED          -10008
#define SPIFFS_ERR_FILE_DELETED         -10009
#define SPIFFS_ERR_BAD_DESCRIPTOR       -10010
#define SPIFFS_ERR_IS_INDEX             -10011
#define SPIFFS_ERR_IS_FREE              -10012
#define SPIFFS_ERR_INDEX_SPAN_MISMATCH  -10013
#define SPIFFS_ERR_DATA_SPAN_MISMATCH   -10014
#define SPIFFS_ERR_INDEX_REF_FREE       -10015
#define SPIFFS_ERR_INDEX_REF_LU         -10016
#define SPIFFS_ERR_INDEX_REF_INVALID    -10017
#define SPIFFS_ERR_INDEX_FREE           -10018
#define SPIFFS_ERR_INDEX_LU             -10019
#define SPIFFS_ERR_INDEX_INVALID        -10020
#define SPIFFS_ERR_NOT_WRITABLE         -10021
#define SPIFFS_ERR_NOT_READABLE         -10022
#define SPIFFS_ERR_CONFLICTING_NAME     -10023
#define SPIFFS_ERR_NOT_CONFIGURED       -10024

#define SPIFFS_ERR_NOT_A_FS             -10025
#define SPIFFS_ERR_MOUNTED              -10026
#define SPIFFS_ERR_ERASE_FAIL           -10027
#define SPIFFS_ERR_MAGIC_NOT_POSSIBLE   -10028

#define SPIFFS_ERR_NO_DELETED_BLOCKS    -10029

#define SPIFFS_ERR_FILE_EXISTS          -10030

#define SPIFFS_ERR_NOT_A_FILE           -10031
#define SPIFFS_ERR_RO_NOT_IMPL          -10032
#define SPIFFS_ERR_RO_ABORTED_OPERATION -10033
#define SPIFFS_ERR_PROBE_TOO_FEW_BLOCKS -10034
#define SPIFFS_ERR_PROBE_NOT_A_FS       -10035
#define SPIFFS_ERR_NAME_TOO_LONG        -10036

#define SPIFFS_ERR_IX_MAP_UNMAPPED      -10037
#define SPIFFS_ERR_IX_MAP_MAPPED        -10038
#define SPIFFS_ERR_IX_MAP_BAD_RANGE     -10039

#define SPIFFS_ERR_SEEK_BOUNDS          -10040


#define SPIFFS_ERR_INTERNAL             -10050

#define SPIFFS_ERR_TEST                 -10100

/*********************************************************************************************************
 * SPIFFS������������
*********************************************************************************************************/
typedef INT16   SPIFFS_FILE;        /* SPIFFS�ļ�������������Ϊ������ */
typedef UINT16  SPIFFS_FLAGS;       /* SPIFFS��־λ */
typedef UINT16  SPIFFS_MODE;        /* SPIFFS�ļ����� */
typedef UINT8   SPIFFS_OBJ_TYPE;    /* SPIFFS Object���ͣ���ΪFile��Index��Data�� */

struct spiffs_volume;
struct spiffs_config;

/* һЩ����ָ�� */
/* spi read call function type */
typedef INT32 (*spiffsRead)(UINT32 uiAddr, UINT32 uiSize, PUCHAR pucDst);
/* spi write call function type */
typedef INT32 (*spiffsWrite)(UINT32 uiAddr, UINT32 uiSize, PUCHAR pucSrc);
/* spi erase call function type */
typedef INT32 (*spiffsErase)(UINT32 uiAddr, UINT32 uiSize);

/* �ļ�ϵͳ�������ͼ�� */
typedef enum spiffs_check_type
{
    SPIFFS_CHECK_LOOKUP = 0,
    SPIFFS_CHECK_INDEX,
    SPIFFS_CHECK_PAGE
} SPIFFS_CHECK_TYPE;

/* �ļ�ϵͳ��鷵��ֵ */
typedef enum spiffs_check_report{
  SPIFFS_CHECK_PROGRESS = 0,
  SPIFFS_CHECK_ERROR,
  SPIFFS_CHECK_FIX_INDEX,
  SPIFFS_CHECK_FIX_LOOKUP,
  SPIFFS_CHECK_DELETE_ORPHANED_INDEX,
  SPIFFS_CHECK_DELETE_PAGE,
  SPIFFS_CHECK_DELETE_BAD_FILE
} SPIFFS_CHECK_REPORT;

typedef enum spiffs_fileop_type{
  /* the file has been created */
  SPIFFS_CB_CREATED = 0,
  /* the file has been updated or moved to another page */
  SPIFFS_CB_UPDATED,
  /* the file has been deleted */
  SPIFFS_CB_DELETED
} SPIFFS_FILEOP_TYPE;

typedef VOID (*spiffsCheckCallback)(SPIFFS_CHECK_TYPE type, SPIFFS_CHECK_REPORT report,
                                    UINT32 arg1, UINT32 arg2);

typedef VOID (*spiffsFileCallback)(struct spiffs_volume* pfs, SPIFFS_FILEOP_TYPE op, 
                                   SPIFFS_OBJ_ID objID, SPIFFS_PAGE_IX pageIX);
   
/*********************************************************************************************************
 * SPIFFS�ļ���������
*********************************************************************************************************/
/* Any write to the filehandle is appended to end of the file */
#define SPIFFS_APPEND                   (1<<0)
#define SPIFFS_O_APPEND                 SPIFFS_APPEND
/* If the opened file exists, it will be truncated to zero length before opened */
#define SPIFFS_TRUNC                    (1<<1)
#define SPIFFS_O_TRUNC                  SPIFFS_TRUNC
/* If the opened file does not exist, it will be created before opened */
#define SPIFFS_CREAT                    (1<<2)
#define SPIFFS_O_CREAT                  SPIFFS_CREAT
/* The opened file may only be read */
#define SPIFFS_RDONLY                   (1<<3)
#define SPIFFS_O_RDONLY                 SPIFFS_RDONLY
/* The opened file may only be written */
#define SPIFFS_WRONLY                   (1<<4)
#define SPIFFS_O_WRONLY                 SPIFFS_WRONLY
/* The opened file may be both read and written */
#define SPIFFS_RDWR                     (SPIFFS_RDONLY | SPIFFS_WRONLY)
#define SPIFFS_O_RDWR                   SPIFFS_RDWR
/* Any writes to the filehandle will never be cached but flushed directly */
#define SPIFFS_DIRECT                   (1<<5)
#define SPIFFS_O_DIRECT                 SPIFFS_DIRECT
/* If SPIFFS_O_CREAT and SPIFFS_O_EXCL are set, SPIFFS_open() shall fail if the file exists */
#define SPIFFS_EXCL                     (1<<6)
#define SPIFFS_O_EXCL                   SPIFFS_EXCL

#define SPIFFS_SEEK_SET                 (0)
#define SPIFFS_SEEK_CUR                 (1)
#define SPIFFS_SEEK_END                 (2)

#define SPIFFS_TYPE_FILE                (1)
#define SPIFFS_TYPE_DIR                 (2)
#define SPIFFS_TYPE_HARD_LINK           (3)
#define SPIFFS_TYPE_SOFT_LINK           (4)

/*********************************************************************************************************
 * SPIFFS������Ϣ
*********************************************************************************************************/
typedef struct spiffs_config{
    // physical read function
    spiffsRead halReadFunc;             /* Ӳ�������� */
    // physical write function
    spiffsWrite halWriteFunc;           /* Ӳ��д���� */
    // physical erase function
    spiffsErase halEraseFunc;           /* Ӳ�������� */
    // physical size of the spi flash
    UINT32 uiPhysSize;                  /* ����Ӳ����С */
    // physical offset in spi flash used for spiffs,
    // must be on block boundary
    UINT32 uiPhysAddr;                  /* ����Ӳ����ʼƫ�� */
    // physical size when erasing a block
    UINT32 uiPhysEraseBlkSize;          /* ����Ӳ���������С */
    // logical size of a block, must be on physical
    // block size boundary and must never be less than
    // a physical block
    UINT32 uiLogicBlkSize;              /* �߼����С */
    // logical size of a page, must be at least
    // log_block_size / 8
    UINT32 uiLogicPageSize;             /* �߼�ҳ���С */
} SPIFFS_CONFIG;
/*********************************************************************************************************
 * SPIFFS�ļ�ͷ����Ϣ����
*********************************************************************************************************/
typedef struct spiffs_volume
{
    // file system configuration
    SPIFFS_CONFIG       cfg;                    /* �����ļ� */
    // number of logical blocks
    LW_OBJECT_HANDLE    hVolLock;               /*  �������                    */
    UINT32              uiBlkCount;                  /* �߼������ */
    // cursor for free blocks, block index
    SPIFFS_BLOCK_IX     blkIXFreeCursor;    /* Free Block������ */
    // cursor for free blocks, entry index (lu => lookup)
    INT                 objLookupEntryFreeCursor;       /* Free Block��LookUp Entry���� */
    // cursor when searching, block index
    SPIFFS_BLOCK_IX     blkIXCursor;        /* Block�������� */
    // cursor when searching, entry index
    INT                 objLookupEntryCursor;           /* Lookup Entry�������� */

    // primary work buffer, size of a logical page
    PUCHAR              pucLookupWorkBuffer;         /* ����LookUp Entry����СΪһ���߼�ҳ�� */
    // secondary work buffer, size of a logical page
    PUCHAR              pucWorkBuffer;               /* �������ҳ�棬��СΪһ���߼�ҳ�� */
    // file descriptor memory area
    UINT8               *puiFdSpace;                  /* �ļ��������洢λ�� */
    // available file descriptors
    UINT32              uiFdCount;                   /* ���õ��ļ������� */

    // last error
    INT32               uiErrorCode;                  /* SPIFFS��һ�������� */

    // current number of free blocks
    UINT32              uiFreeBlks;                  /* ���п� */
    // current number of busy pages
    UINT32              uiStatsPageAllocated;        /* ��ǰ�ѷ����ҳ���� */
    // current number of deleted pages
    UINT32              uiStatsPageDeleted;          /* ��ǰ��ɾ����ҳ���� */
    // flag indicating that garbage collector is cleaning
    UINT8               uiCleaningFlag;               /* ��־�ɾ���GCʹ��? */
    // max erase count amongst all blocks
    SPIFFS_OBJ_ID       uiMaxEraseCount;      /* ����Block������EraseCount */
    //TODO:����ģ�
    UINT32              uiStatsGCRuns;               //! ???
    
    // cache memory
    PVOID               pCache;                       /* Cache�ڴ�ռ� */           
    // cache size
    UINT32              uiCacheSize;                 /* Cache��С */
    UINT32              uiCacheHits;                 /* Cache���д��� */
    UINT32              uiCacheMisses;               /* Cacheȱʧ���� */

    //TODO:��ʱû������ʲô��
    // check callback function       
    spiffsCheckCallback checkCallbackFunc;
    // file callback function
    spiffsFileCallback  fileCallbackFunc;
    
    // mounted flag
    UINT8               uiMountedFlag;                /* �Ƿ���أ� */
    // user data
    //TODO:��ʲô��
    PVOID               pUserData;                    /* �û����� */
    // config magic
    UINT32              uiConfigMagic;               /* ���� */
} SPIFFS_VOLUME;

typedef SPIFFS_VOLUME * PSPIFFS_VOLUME;
typedef SPIFFS_CONFIG * PSPIFFS_CONFIG;

/*********************************************************************************************************
 * SPIFFS�ļ�״̬����
*********************************************************************************************************/
typedef struct spiffs_stat{
  SPIFFS_OBJ_ID objID;
  UINT32 uiSize;
  SPIFFS_OBJ_TYPE objType;
  SPIFFS_PAGE_IX pageIX;
  UCHAR name[SPIFFS_OBJ_NAME_LEN];
} SPIFFS_STAT;
/*********************************************************************************************************
 * SPIFFSĿ¼�ļ�״̬
*********************************************************************************************************/
typedef struct spiffs_dirent{
  SPIFFS_OBJ_ID objID;
  UINT32 uiSize;
  SPIFFS_OBJ_TYPE objType;
  SPIFFS_PAGE_IX pageIX;
  UCHAR name[SPIFFS_OBJ_NAME_LEN];
} SPIFFS_DIRENT;
/*********************************************************************************************************
 * SPIFFSĿ¼����
*********************************************************************************************************/
typedef struct {
  PSPIFFS_VOLUME *pfs;
  SPIFFS_BLOCK_IX blkIX;
  INT uiEntry;
} spiffs_DIR;

/*********************************************************************************************************
 * SPIFFS Index Page�ڴ�ӳ��ṹ
*********************************************************************************************************/
typedef struct {
  // buffer with looked up data pixes
  SPIFFS_PAGE_IX *pPageIXMapBuffer;
  // precise file byte offset
  UINT32 uiOffset;
  // start data span index of lookup buffer
  SPIFFS_SPAN_IX spanIXStart;
  // end data span index of lookup buffer
  SPIFFS_SPAN_IX spanIXEnd;
} spiffs_ix_map;



/*********************************************************************************************************
 * SPIFFS �ϲ㺯������
*********************************************************************************************************/
INT32 __spiffs_mount(PSPIFFS_VOLUME pfs, PSPIFFS_CONFIG pConfig, PUCHAR pucWorkBuffer,
                     UINT8 *puiFdSpace, UINT32 uiFdSpaceSize,
                     VOID *pCache, UINT32 uiCacheSize,
                     spiffsCheckCallback checkCallbackFunc);

VOID __spiffs_unmount(PSPIFFS_VOLUME pfs);




#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFFSTYPE_H_ */
