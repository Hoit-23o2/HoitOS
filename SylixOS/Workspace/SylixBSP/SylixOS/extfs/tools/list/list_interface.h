/*
 * list_interface.h
 *
 *  Created on: Jun 2, 2021
 *      Author: Administrator
 */

#ifndef SYLIXOS_EXTFS_TOOLS_LIST_LIST_INTERFACE_H_
#define SYLIXOS_EXTFS_TOOLS_LIST_LIST_INTERFACE_H_
#include "list_template.h"
/********************************************************************************************************* 
  列表接口声明                                                                                          
*********************************************************************************************************/ 
#define List(TYPE) List##TYPE*
#define InitList(list, TYPE)\
{\
    list = (List##TYPE *)lib_malloc(sizeof(List##TYPE));\
    list->listHeader.next = LW_NULL;\
    list->listHeader.prev = LW_NULL;\
    list->append = listAppend##TYPE;\
    list->insert = listInsert##TYPE;\
    list->removeIndex = listRemoveIndex##TYPE;\
    list->removeObject = listRemoveObject##TYPE;\
    list->size = listSize##TYPE;\
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
#define InitIterator(iter, TYPE)\
{\
    iter = (Iterator##TYPE*)lib_malloc(sizeof(Iterator##TYPE));\
    iter->begin = iterBegin##TYPE;\
    iter->get = iterGet##TYPE;\
    iter->isValid = iterIsValid##TYPE;\
    iter->next = iterNext##TYPE;\
}
#define FreeIterator(iter) lib_free(iter);

#endif /* SYLIXOS_EXTFS_TOOLS_LIST_LIST_INTERFACE_H_ */
