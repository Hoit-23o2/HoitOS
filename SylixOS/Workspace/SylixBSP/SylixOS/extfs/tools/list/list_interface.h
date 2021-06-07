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
** 文   件   名: list_interface.h
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 02日
**
** 描        述: 链表接口
*********************************************************************************************************/


#ifndef SYLIXOS_EXTFS_TOOLS_LIST_LIST_INTERFACE_H_
#define SYLIXOS_EXTFS_TOOLS_LIST_LIST_INTERFACE_H_
#include "list_template.h"
/*********************************************************************************************************
  列表接口声明
*********************************************************************************************************/
#define List(TYPE) List##TYPE*
#define InitList(list, NAMESPACE, TYPE)\
{\
    list = (List##TYPE *)lib_malloc(sizeof(List##TYPE));\
    list->listHeader.next = LW_NULL;\
    list->listHeader.prev = LW_NULL;\
    list->append = NAMESAPCE##listAppend##TYPE;\
    list->insert = NAMESAPCE##listInsert##TYPE;\
    list->removeIndex = NAMESAPCE##listRemoveIndex##TYPE;\
    list->removeObject = NAMESAPCE##listRemoveObject##TYPE;\
    list->size = NAMESAPCE##listSize##TYPE;\
}
#define FreeList(list)\
{\
    UINT uiSize = list->uiSize;\
    UINT i;\
    for (i = 0; i < uiSize; i++)\
    {\
        list->removeIndex(list, 0);\
    }\
    lib_free(list);\
}
/*********************************************************************************************************
  迭代器声明
*********************************************************************************************************/
#define Iterator(TYPE) Iterator##TYPE*
#define InitIterator(iter,NAMESPACE, TYPE)\
{\
    iter = (Iterator##TYPE*)lib_malloc(sizeof(Iterator##TYPE));\
    iter->begin = NAMESAPCE##iterBegin##TYPE;\
    iter->get = NAMESAPCE##iterGet##TYPE;\
    iter->isValid = NAMESAPCE##iterIsValid##TYPE;\
    iter->next = NAMESAPCE##iterNext##TYPE;\
}
#define FreeIterator(iter) lib_free(iter);

#endif /* SYLIXOS_EXTFS_TOOLS_LIST_LIST_INTERFACE_H_ */
