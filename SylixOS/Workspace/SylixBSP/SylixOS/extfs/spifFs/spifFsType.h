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
** 文   件   名: spifFsType.h
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 01日
**
** 描        述: Spiffs文件系统类型
*********************************************************************************************************/

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL

#ifndef SYLIXOS_EXTFS_SPIFFS_SPIFFSTYPE_H_
#define SYLIXOS_EXTFS_SPIFFS_SPIFFSTYPE_H_
#include "spifFsConfig.h"
/*********************************************************************************************************
 * 相关宏定义
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
 * SPIFFS基本数据类型
*********************************************************************************************************/
typedef INT16   SPIFFS_FILE;        /* SPIFFS文件描述符，必须为带符号 */
typedef UINT16  SPIFFS_FLAGS;       /* SPIFFS标志位 */
typedef UINT16  SPIFFS_MODE;        /* SPIFFS文件类型 */
typedef UINT8   SPIFFS_OBJ_TYPE;    /* SPIFFS Object类型，可为File，Index，Data等 */

struct spiffs_volume;
struct spiffs_config;

/* 一些函数指针 */
/* spi read call function type */
typedef INT32 (*spiffsRead)(UINT32 uiAddr, UINT32 uiSize, PUCHAR pucDst);
/* spi write call function type */
typedef INT32 (*spiffsWrite)(UINT32 uiAddr, UINT32 uiSize, PUCHAR pucSrc);
/* spi erase call function type */
typedef INT32 (*spiffsErase)(UINT32 uiAddr, UINT32 uiSize);

/* 文件系统定义类型检查 */
typedef enum spiffs_check_type
{
    SPIFFS_CHECK_LOOKUP = 0,
    SPIFFS_CHECK_INDEX,
    SPIFFS_CHECK_PAGE
} SPIFFS_CHECK_TYPE;

/* 文件系统检查返回值 */
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
 * SPIFFS文件操作类型
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
 * SPIFFS配置信息
*********************************************************************************************************/
typedef struct spiffs_config{
    // physical read function
    spiffsRead halReadFunc;             /* 硬件读函数 */
    // physical write function
    spiffsWrite halWriteFunc;           /* 硬件写函数 */
    // physical erase function
    spiffsErase halEraseFunc;           /* 硬件擦函数 */
    // physical size of the spi flash
    UINT32 uiPhysSize;                  /* 物理硬件大小 */
    // physical offset in spi flash used for spiffs,
    // must be on block boundary
    UINT32 uiPhysAddr;                  /* 物理硬件起始偏移 */
    // physical size when erasing a block
    UINT32 uiPhysEraseBlkSize;          /* 物理硬件擦除块大小 */
    // logical size of a block, must be on physical
    // block size boundary and must never be less than
    // a physical block
    UINT32 uiLogicBlkSize;              /* 逻辑块大小 */
    // logical size of a page, must be at least
    // log_block_size / 8
    UINT32 uiLogicPageSize;             /* 逻辑页面大小 */
} SPIFFS_CONFIG;
/*********************************************************************************************************
 * SPIFFS文件头等信息定义
*********************************************************************************************************/
typedef struct spiffs_volume
{
    // file system configuration
    SPIFFS_CONFIG       cfg;                    /* 配置文件 */
    // number of logical blocks
    LW_OBJECT_HANDLE    hVolLock;               /*  卷操作锁                    */
    UINT32              uiBlkCount;                  /* 逻辑块个数 */
    // cursor for free blocks, block index
    SPIFFS_BLOCK_IX     blkIXFreeCursor;    /* Free Block的索引 */
    // cursor for free blocks, entry index (lu => lookup)
    INT                 objLookupEntryFreeCursor;       /* Free Block的LookUp Entry索引 */
    // cursor when searching, block index
    SPIFFS_BLOCK_IX     blkIXCursor;        /* Block遍历索引 */
    // cursor when searching, entry index
    INT                 objLookupEntryCursor;           /* Lookup Entry遍历索引 */

    // primary work buffer, size of a logical page
    PUCHAR              pucLookupWorkBuffer;         /* 缓存LookUp Entry，大小为一个逻辑页面 */
    // secondary work buffer, size of a logical page
    PUCHAR              pucWorkBuffer;               /* 缓存各种页面，大小为一个逻辑页面 */
    // file descriptor memory area
    UINT8               *puiFdSpace;                  /* 文件描述符存储位置 */
    // available file descriptors
    UINT32              uiFdCount;                   /* 可用的文件描述符 */

    // last error
    INT32               uiErrorCode;                  /* SPIFFS上一个错误码 */

    // current number of free blocks
    UINT32              uiFreeBlks;                  /* 空闲块 */
    // current number of busy pages
    UINT32              uiStatsPageAllocated;        /* 当前已分配的页面数 */
    // current number of deleted pages
    UINT32              uiStatsPageDeleted;          /* 当前已删除的页面数 */
    // flag indicating that garbage collector is cleaning
    UINT8               uiCleaningFlag;               /* 标志干净，GC使用? */
    // max erase count amongst all blocks
    SPIFFS_OBJ_ID       uiMaxEraseCount;      /* 所有Block中最大的EraseCount */
    //TODO:干嘛的？
    UINT32              uiStatsGCRuns;               //! ???
    
    // cache memory
    PVOID               pCache;                       /* Cache内存空间 */           
    // cache size
    UINT32              uiCacheSize;                 /* Cache大小 */
    UINT32              uiCacheHits;                 /* Cache命中次数 */
    UINT32              uiCacheMisses;               /* Cache缺失次数 */

    //TODO:暂时没发现有什么用
    // check callback function       
    spiffsCheckCallback checkCallbackFunc;
    // file callback function
    spiffsFileCallback  fileCallbackFunc;
    
    // mounted flag
    UINT8               uiMountedFlag;                /* 是否挂载？ */
    // user data
    //TODO:有什么用
    PVOID               pUserData;                    /* 用户数据 */
    // config magic
    UINT32              uiConfigMagic;               /* 幻数 */
} SPIFFS_VOLUME;

typedef SPIFFS_VOLUME * PSPIFFS_VOLUME;
typedef SPIFFS_CONFIG * PSPIFFS_CONFIG;

/*********************************************************************************************************
 * SPIFFS文件状态描述
*********************************************************************************************************/
typedef struct spiffs_stat{
  SPIFFS_OBJ_ID objID;
  UINT32 uiSize;
  SPIFFS_OBJ_TYPE objType;
  SPIFFS_PAGE_IX pageIX;
  UCHAR name[SPIFFS_OBJ_NAME_LEN];
} SPIFFS_STAT;
/*********************************************************************************************************
 * SPIFFS目录文件状态
*********************************************************************************************************/
typedef struct spiffs_dirent{
  SPIFFS_OBJ_ID objID;
  UINT32 uiSize;
  SPIFFS_OBJ_TYPE objType;
  SPIFFS_PAGE_IX pageIX;
  UCHAR name[SPIFFS_OBJ_NAME_LEN];
} SPIFFS_DIRENT;
/*********************************************************************************************************
 * SPIFFS目录描述
*********************************************************************************************************/
typedef struct {
  PSPIFFS_VOLUME *pfs;
  SPIFFS_BLOCK_IX blkIX;
  INT uiEntry;
} spiffs_DIR;

/*********************************************************************************************************
 * SPIFFS Index Page内存映射结构
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
 * SPIFFS 上层函数定义
*********************************************************************************************************/
INT32 __spiffs_mount(PSPIFFS_VOLUME pfs, PSPIFFS_CONFIG pConfig, PUCHAR pucWorkBuffer,
                     UINT8 *puiFdSpace, UINT32 uiFdSpaceSize,
                     VOID *pCache, UINT32 uiCacheSize,
                     spiffsCheckCallback checkCallbackFunc);

VOID __spiffs_unmount(PSPIFFS_VOLUME pfs);




#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFFSTYPE_H_ */
