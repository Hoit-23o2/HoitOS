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

#ifndef SYLIXOS_EXTFS_SPIFFS_SPIFFSTYPE_H_
#define SYLIXOS_EXTFS_SPIFFS_SPIFFSTYPE_H_

#include "SylixOS.h"
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

#define SPIFFS_ERR_CACHE_NO_INIT        -10200
#define SPIFFS_ERR_CACHE_NO_MEM         -10201
#define SPIFFS_ERR_CACHE_OVER_RD        -10202
/*********************************************************************************************************
 * 一些标志位
*********************************************************************************************************/
#define SPIFFS_OBJ_ID_IX_FLAG           ((SPIFFS_OBJ_ID)(1UL<<(8*sizeof(SPIFFS_OBJ_ID)-1)))

#define SPIFFS_OBJ_ID_DELETED           ((SPIFFS_OBJ_ID)0)
#define SPIFFS_OBJ_ID_FREE              ((SPIFFS_OBJ_ID)-1)         /* Norflash干净的时候均为1 */

/*********************************************************************************************************
 * 一些操作标志宏
*********************************************************************************************************/
#define SPIFFS_OP_T_OBJ_LU    (0<<0)    /* 00 */
#define SPIFFS_OP_T_OBJ_LU2   (1<<0)    /* 01 不缓存读取*/
#define SPIFFS_OP_T_OBJ_IX    (2<<0)    /* 10 */
#define SPIFFS_OP_T_OBJ_DA    (3<<0)    /* 11 */
#define SPIFFS_OP_TYPE_MASK   (3<<0)    /* 11 */

#define SPIFFS_OP_C_DELE      (0<<2)    /* 000/00 */
#define SPIFFS_OP_C_UPDT      (1<<2)    /* 001/00 */
#define SPIFFS_OP_C_MOVS      (2<<2)    /* 010/00 Move Src*/
#define SPIFFS_OP_C_MOVD      (3<<2)    /* 011/00 Move To Dest*/
#define SPIFFS_OP_C_FLSH      (4<<2)    /* 100/00 */
#define SPIFFS_OP_C_READ      (5<<2)    /* 101/00 */
#define SPIFFS_OP_C_WRTHRU    (6<<2)    /* 110/00 */
#define SPIFFS_OP_COM_MASK    (7<<2)    /* 111/00 */

/*********************************************************************************************************
 * 页面标志位
*********************************************************************************************************/
// if 0, this page is written to, else clean
#define SPIFFS_PH_FLAG_USED   (1<<0)
// if 0, writing is finalized, else under modification
#define SPIFFS_PH_FLAG_FINAL  (1<<1)
// if 0, this is an index page, else a data page
#define SPIFFS_PH_FLAG_INDEX  (1<<2)
// if 0, page is deleted, else valid
#define SPIFFS_PH_FLAG_DELET  (1<<7)
// if 0, this index header is being deleted
#define SPIFFS_PH_FLAG_IXDELE (1<<6)
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
 * SPIFFS基本数据类型
*********************************************************************************************************/
typedef INT16   SpiffsFile;       /* SPIFFS文件描述符，必须为带符号 */
typedef UINT16  SpiffsFlags;      /* SPIFFS标志位 */
typedef UINT16  SpiffsMode;       /* SPIFFS文件类型 */
typedef UINT8   SpiffsObjType;    /* SPIFFS Object类型，可为File，Index，Data等 */

/* 文件系统定义类型检查 */
typedef enum spiffs_check_type
{
    SPIFFS_CHECK_LOOKUP,
    SPIFFS_CHECK_INDEX,
    SPIFFS_CHECK_PAGE
} SPIFFS_CHECK_TYPE;



struct AS
{
    int a;
    int b;
    int c;
};

#define INIT_NODE(type) struct node##type \
{   \
    struct node * next;  \
    int b; \
    type data;\
} ;
INIT_NODE(PCHAR);



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
typedef SPIFFS_STAT * PSPIFFS_STAT;
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
typedef SPIFFS_DIRENT * PSPIFFS_DIRENT;
/*********************************************************************************************************
 * SPIFFS目录描述
*********************************************************************************************************/
typedef struct spiffs_DIR{
    PSPIFFS_VOLUME *pfs;
    SPIFFS_BLOCK_IX blkIX;
    INT uiEntry;
} SPIFFS_DIR;
typedef SPIFFS_DIR * PSPIFFS_DIR;
/*********************************************************************************************************
 * SPIFFS Index Page内存映射结构
*********************************************************************************************************/
typedef struct spiffs_ix_map{
    // buffer with looked up data pixes
    SPIFFS_PAGE_IX *pPageIXMapBuffer;
    // precise file byte offset
    UINT32 uiOffset;
    // start data span index of lookup buffer
    SPIFFS_SPAN_IX spanIXStart;
    // end data span index of lookup buffer
    SPIFFS_SPAN_IX spanIXEnd;
} SPIFFS_IX_MAP;
typedef SPIFFS_IX_MAP * PSPIFFS_IX_MAP;
/*********************************************************************************************************
 * SPIFFS Cache Page结构
*********************************************************************************************************/
typedef struct spiffs_cache_page{
    // cache flags
    UINT8 flags;                            /* Cache的类型，供Union使用 */
    // cache page index
    UINT8 uiIX;                             /* 在Cache中的Index */
    // last access of this cache page
    UINT32 uiLastAccess;                    /* 这个Cache Page上次访问时间 */
    //TODO: 这是什么捏？
    union {
      // type Read cache
      struct {
        // read cache page index
        SPIFFS_PAGE_IX pageIX;
      };
      // type Write cache
      struct {
        // write cache
        SPIFFS_OBJ_ID objId;
        // offset in cache page
        UINT32 uiOffset;
        // size of cache page
        UINT16 uiSize;
      };
    };
} SPIFFS_CACHE_PAGE;
typedef SPIFFS_CACHE_PAGE * PSPIFFS_CACHE_PAGE; 
/*********************************************************************************************************
 * SPIFFS Cache 结构
*********************************************************************************************************/
// cache struct
typedef struct spiffs_cache{
    UINT8   uiCpageCount;       /* 页面数 */
    UINT32  uiLastAccess;       /* 上一次访问 */
    UINT32  uiCpageUseMap;      /* CachePage使用位图 */
    UINT32  uiCpageUseMask;     /* 代表总共可容纳的Cache Page */
    PUCHAR  Cpages;             /* 页面缓存 */
} SPIFFS_CACHE;
typedef SPIFFS_CACHE * PSPIFFS_CACHE; 
/*********************************************************************************************************
 * SPIFFS 文件描述符结构
*********************************************************************************************************/
// spiffs nucleus file descriptor
typedef struct spiffs_fd{
    // the filesystem of this descriptor
    PSPIFFS_VOLUME pfs;
    // number of file descriptor - if 0, the file descriptor is closed
    SPIFFS_FILE fileN;
    // object id - if SPIFFS_OBJ_ID_ERASED, the file was deleted
    SPIFFS_OBJ_ID objId;
    // size of the file
    UINT32 uiSize;
    // cached object index header page index
    SPIFFS_PAGE_IX pageIXObjIXHdr;
    // cached offset object index page index
    SPIFFS_PAGE_IX pageIXObjIXCursor;
    // cached offset object index span index
    SPIFFS_SPAN_IX spanIXObjIXCursor;
    // current absolute offset
    UINT32 uiOffset;
    // current file descriptor offset (cached)
    UINT32 uiFdOffset;
    // fd flags
    SPIFFS_FLAGS flags;
    PSPIFFS_CACHE_PAGE pCachePage;
    // djb2 hash of filename
    UINT32 uiNameHash;
    // hit score (score == 0 indicates never used fd)
    UINT16 uiScore;
    // spiffs index map, if 0 it means unmapped
    PSPIFFS_IX_MAP pIXMap;
} SPIFFS_FD;
typedef SPIFFS_FD * PSPIFFS_FD;
/*********************************************************************************************************
 * SPIFFS 普通页面内容头部
*********************************************************************************************************/
typedef struct spiffs_page_header {
  // object id
  SPIFFS_OBJ_ID objId;
  // object span index
  SPIFFS_SPAN_IX spanIX;
  // flags
  UINT8 flags;
} SPIFFS_PAGE_HEADER;
typedef SPIFFS_PAGE_HEADER * PSPIFFS_PAGE_HEADER;
/*********************************************************************************************************
 * SPIFFS IX页面内容头部
*********************************************************************************************************/
typedef struct spiffs_page_object_ix_header         /* SpanIndex为0时，即Index header */
{
  // common page header
  SPIFFS_PAGE_HEADER pageHdr;
  // alignment
  UINT8 __align[4 - ((sizeof(SPIFFS_PAGE_HEADER) & 3) == 0 ? 4 : (sizeof(SPIFFS_PAGE_HEADER) & 3))];
  // size of object
  UINT32 uiSize;
  // type of object
  SPIFFS_OBJ_TYPE type;
  // name of object
  UCHAR ucName[SPIFFS_OBJ_NAME_LEN];
} SPIFFS_PAGE_OBJECT_IX_HEADER;

// object index page header
typedef struct spiffs_page_object_ix {
 SPIFFS_PAGE_HEADER pageHdr;
 UINT8 __align[4 - ((sizeof(SPIFFS_PAGE_HEADER) & 3) == 0 ? 4 : (sizeof(SPIFFS_PAGE_HEADER) & 3))];
} SPIFFS_PAGE_OBJECT_IX;
typedef SPIFFS_PAGE_OBJECT_IX * PSPIFFS_PAGE_OBJECT_IX;
/*********************************************************************************************************
 * 寻找FreeObjId时运用到状态结构体
*********************************************************************************************************/
typedef struct spiffs_free_obj_id_state{
  SPIFFS_OBJ_ID objIdMin;
  SPIFFS_OBJ_ID objIdMax;
  UINT32 uiCompaction;
  const PUCHAR pucConflictingName;
} SPIFFS_FREE_OBJ_ID_STATE;
typedef SPIFFS_FREE_OBJ_ID_STATE * PSPIFFS_FREE_OBJ_ID_STATE;
/*********************************************************************************************************
 * 算术宏
*********************************************************************************************************/
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
/*********************************************************************************************************
 * SPIFFS 文件系统文件头相关宏
*********************************************************************************************************/
#define SPIFFS_CONFIG_MAGIC             (0x52AA1314)        /* AA = A星星 */

#define SPIFFS_CFG_LOGIC_PAGE_SZ(pfs)               ((pfs)->cfg.uiLogicPageSize)
#define SPIFFS_CFG_LOGIC_BLOCK_SZ(pfs)              ((pfs)->cfg.uiLogicBlkSize)
#define SPIFFS_CFG_PHYS_SZ(pfs)                     ((pfs)->cfg.uiPhysSize)
#define SPIFFS_CFG_PHYS_ERASE_SZ(pfs)               ((pfs)->cfg.uiPhysEraseBlkSize)
#define SPIFFS_CFG_PHYS_ADDR(pfs)                   ((pfs)->cfg.uiPhysAddr)

/* 每个块有不同的 MAGIC NUM */
#define SPIFFS_MAGIC(pfs, blkIX)                    ((SPIFFS_OBJ_ID)(SPIFFS_CONFIG_MAGIC ^ SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) ^\
                                                    ((pfs)->uiBlkCount - (blkIX))))
#define SPIFFS_UNDEFINED_LEN            (UINT32)(-1)
/*********************************************************************************************************
 * SPIFFS 地址转换相关宏
*********************************************************************************************************/
#define SPIFFS_PADDR_TO_PAGE(pfs, uiPhysAddr)           (((uiPhysAddr) -  SPIFFS_CFG_PHYS_ADDR(pfs)) / SPIFFS_CFG_LOGIC_PAGE_SZ(pfs))
#define SPIFFS_PADDR_TO_PAGE_OFFSET(pfs, uiPhysAddr)    (((uiPhysAddr) - SPIFFS_CFG_PHYS_ADDR(pfs)) % SPIFFS_CFG_LOGIC_PAGE_SZ(pfs))
#define SPIFFS_PAGE_TO_PADDR(pfs, pageIX)               (SPIFFS_CFG_PHYS_ADDR(pfs) + (pageIX) * SPIFFS_CFG_LOGIC_PAGE_SZ(pfs))

#define NODE_CAST(dst,src,type) struct node##type \
{   \
    struct node * next;  \
    int b; \
    type data;\
}; dst = (struct node##type *)src

#define SPIFFS_BLOCK_TO_PADDR(pfs, blkIX)               (SPIFFS_CFG_PHYS_ADDR(pfs) + (blkIX)* SPIFFS_CFG_LOGIC_BLOCK_SZ(pfs))
#define SPIFFS_PAGES_PER_BLOCK(pfs)                     (SPIFFS_CFG_LOGIC_BLOCK_SZ(pfs) / SPIFFS_CFG_LOGIC_PAGE_SZ(pfs))
/* 将页面转化为所在块号 */
#define SPIFFS_BLOCK_FOR_PAGE(pfs, pageIX)              ((pageIX) / SPIFFS_PAGES_PER_BLOCK(pfs))
/*********************************************************************************************************
 * SPIFFS Look Up Page相关
*********************************************************************************************************/
/* LOOK UP page占有的页数 */
#define SPIFFS_OBJ_LOOKUP_PAGES(pfs)                    (MAX(1, (SPIFFS_PAGES_PER_BLOCK(pfs) * sizeof(SPIFFS_OBJ_ID)) \
                                                        / SPIFFS_CFG_LOGIC_PAGE_SZ(pfs)) )
/* 一个Blk的最大Entry数 */
#define SPIFFS_OBJ_LOOKUP_MAX_ENTRIES(pfs)              (SPIFFS_PAGES_PER_BLOCK(pfs) - SPIFFS_OBJ_LOOKUP_PAGES(pfs))
#define SPIFFS_OBJ_LOOKUP_ENTRY_TO_PIX(pfs, blkIX, iEntry)\
                                                        ((blkIX)*SPIFFS_PAGES_PER_BLOCK(pfs) + (SPIFFS_OBJ_LOOKUP_PAGES(pfs) + iEntry))
/* 倒数第二个Lookup Entry为块的擦写次数 */
#define SPIFFS_ERASE_COUNT_PADDR(pfs, blkIX)            (SPIFFS_BLOCK_TO_PADDR(pfs, blkIX) + SPIFFS_OBJ_LOOKUP_PAGES(pfs) *\
                                                        SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) - sizeof(SPIFFS_OBJ_ID))
/* 倒数第二个Lookup Entry为Blk的MagicNum */
#define SPIFFS_MAGIC_PADDR(pfs, blkIX)                  (SPIFFS_BLOCK_TO_PADDR(pfs, blkIX) + SPIFFS_OBJ_LOOKUP_PAGES(pfs) *\
                                                        SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) - sizeof(SPIFFS_OBJ_ID) * 2)
/* 对于给定的data page span index或者entry找到Object Index类型的span index */
#define SPIFFS_OBJ_IX_ENTRY_SPAN_IX(pfs, spanIX)        ((spanIX) < SPIFFS_OBJ_HDR_IX_LEN(pfs) ? 0 :\
                                                        (1 + ((spanIX) - SPIFFS_OBJ_HDR_IX_LEN(pfs)) / SPIFFS_OBJ_IX_LEN(pfs)))
/* 将page转化为Entry号 */
#define SPIFFS_OBJ_LOOKUP_ENTRY_FOR_PAGE(pfs, pageIX)   ((pageIX) % SPIFFS_PAGES_PER_BLOCK(pfs) - SPIFFS_OBJ_LOOKUP_PAGES(pfs))
/*********************************************************************************************************
 * SPIFFS IX Page相关
*********************************************************************************************************/
// entries in an object header page index
#define SPIFFS_OBJ_HDR_IX_LEN(pfs)  ((SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) - sizeof(SPIFFS_PAGE_OBJECT_IX_HEADER)) \
                                    / sizeof(SPIFFS_PAGE_IX))
// entries in an object page index
#define SPIFFS_OBJ_IX_LEN(pfs)      ((SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) - sizeof(SPIFFS_PAGE_OBJECT_IX)) \
                                    / sizeof(SPIFFS_PAGE_IX))
#define SPIFFS_OBJ_IX_ENTRY(pfs, spanIX) ((spanIX) < SPIFFS_OBJ_HDR_IX_LEN(pfs) ? (spanIX) \
                                         : (((spanIX) - SPIFFS_OBJ_HDR_IX_LEN(pfs)) % SPIFFS_OBJ_IX_LEN(pfs)))
/*********************************************************************************************************
 * SPIFFS Data Page相关
*********************************************************************************************************/
#define SPIFFS_DATA_PAGE_SIZE(pfs)  (SPIFFS_CFG_LOGIC_PAGE_SZ(pfs) - sizeof(SPIFFS_PAGE_HEADER))


#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFFSTYPE_H_ */
