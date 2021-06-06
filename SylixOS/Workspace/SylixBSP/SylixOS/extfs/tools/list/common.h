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
** 文   件   名: common.h
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 02日
**
** 描        述: 链表测试文件
*********************************************************************************************************/

#ifndef SYLIXOS_EXTFS_TOOLS_LIST_COMMON_H_
#define SYLIXOS_EXTFS_TOOLS_LIST_COMMON_H_
#include "SylixOS.h"
#include "list_interface.h"

typedef struct TYPE {
    int a;
    int b;
} TYPE;

typedef struct node {
    struct node * next;
    struct node * prev;
    TYPE* listData;
} ListNode;

typedef struct list
{
    ListNode listHeader;
    UINT     uiSize;
    UINT     (*size)(struct list* self);
    BOOL     (*append)(struct list* self, TYPE* data);
    BOOL     (*insert)(struct list* self, TYPE* data, UINT uiIndex);
    BOOL     (*removeObject)(struct list* self, TYPE* data);
    BOOL     (*removeIndex)(struct list* self, UINT uiIndex);
} List;

typedef struct iter
{
    ListNode *  traverse;
    VOID        (*begin)(struct iter* self, List* list);
    BOOL        (*next)(struct iter* self);
    BOOL        (*isValid)(struct iter* self);
    TYPE*       (*get)(struct iter* self);
} Iterator;

Iterator* initIterator();
VOID iterBegin(struct iter* self, List* list);
BOOL iterNext(struct iter* self);
BOOL iterIsValid(struct iter* self);
TYPE* iterGet(struct iter* self);
VOID freeIterator(Iterator* iter);

List* initList();
UINT listSize(struct list* self);
BOOL listInsert(struct list* self, TYPE* data, UINT uiIndex);
BOOL listAppend(struct list* self, TYPE* data);
BOOL listRemoveObject(struct list* self, TYPE* data);
BOOL listRemoveIndex(struct list* self, UINT uiIndex);
VOID freeList(List* list);



DECLARE_LIST_TEMPLATE(TYPE);
VOID listTest();


#endif /* SYLIXOS_EXTFS_TOOLS_LIST_COMMON_H_ */
