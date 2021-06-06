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
** 文   件   名: iter.c
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 02日
**
** 描        述: 迭代器原型文件
*********************************************************************************************************/


#ifndef SYLIXOS_EXTFS_TOOLS_LIST_ITER_H_
#define SYLIXOS_EXTFS_TOOLS_LIST_ITER_H_
#include "common.h"

Iterator* initIterator(){
    Iterator* iter = (Iterator*)lib_malloc(sizeof(Iterator));
    iter->begin = iterBegin;
    iter->get = iterGet;
    iter->isValid = iterIsValid;
    iter->next = iterNext;
    return iter;
}

VOID freeIterator(Iterator* iter){
    lib_free(iter);
}

VOID iterBegin(struct iter* self, List* list){
    self->traverse = list->listHeader.next;
}

BOOL iterNext(struct iter* self){
    self->traverse = self->traverse == LW_NULL ? LW_NULL : self->traverse->next;
    return self->traverse == LW_NULL ? LW_FALSE : LW_TRUE;
}

BOOL iterIsValid(struct iter* self) {
    return self->traverse == LW_NULL ? LW_FALSE : LW_TRUE;
}

TYPE* iterGet(struct iter* self){
    return self->traverse->listData;
}

#endif /* SYLIXOS_EXTFS_TOOLS_LIST_ITER_H_ */
