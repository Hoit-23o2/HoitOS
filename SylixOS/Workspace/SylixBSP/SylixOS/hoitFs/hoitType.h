/*
 * hoitType.h
 *
 *  Created on: Mar 26, 2021
 *      Author: Administrator
 */

#ifndef SYLIXOS_EXTFS_HOITFS_HOITTYPE_H_
#define SYLIXOS_EXTFS_HOITFS_HOITTYPE_H_

#include "stdio.h"

/*********************************************************************************************************
  偏移量计算
*********************************************************************************************************/
#define OFFSETOF(type, member)                                      \
        ((size_t)&((type *)0)->member)                              \
/*********************************************************************************************************
  得到ptr的容器结构
*********************************************************************************************************/
#define CONTAINER_OF(ptr, type, member)                             \
        ((type *)((size_t)ptr - OFFSETOF(type, member)))      \



#endif /* SYLIXOS_EXTFS_HOITFS_HOITTYPE_H_ */
