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



#define NODE(type) struct node##type

#define NODE_CAST(dst,src,type) struct node##type \
{   \
    struct node * next;  \
    int b; \
    type data;\
}; dst = (struct node##type *)src

#define MALLOC_PNODE(type) (NODE##type*)lib_malloc(sizeof(NODE##type))


void f(NODE(AS) * a){
    
    //a = MALLOC_PNODE(AS);
    //INSERT(list, a);
}


#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFFSTYPE_H_ */
