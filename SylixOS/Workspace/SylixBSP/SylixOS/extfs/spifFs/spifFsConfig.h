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
** ��   ��   ��: spifFsConfig.h
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 06 �� 01��
**
** ��        ��: Spiffs�ļ�ϵͳ���ò�
*********************************************************************************************************/
#ifndef SYLIXOS_EXTFS_SPIFFS_SPIFFSCONFIG_H_
#define SYLIXOS_EXTFS_SPIFFS_SPIFFSCONFIG_H_
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
/*********************************************************************************************************
  �����������
*********************************************************************************************************/
// some general signed number
#ifndef _SPIPRIi
#define _SPIPRIi   "%d"             /* �������� */
#endif
// address
#ifndef _SPIPRIad
#define _SPIPRIad  "%08x"           /* ��ַ */
#endif
// block
#ifndef _SPIPRIbl
#define _SPIPRIbl  "%04x"           /* Block��� */
#endif
// page
#ifndef _SPIPRIpg
#define _SPIPRIpg  "%04x"           /* ҳ�� */
#endif
// span index
#ifndef _SPIPRIsp
#define _SPIPRIsp  "%04x"           /* Span�� */
#endif
// file descriptor
#ifndef _SPIPRIfd
#define _SPIPRIfd  "%d"             /* �ļ������� */
#endif
// file object id
#ifndef _SPIPRIid
#define _SPIPRIid  "%04x"           /* Object�� */
#endif
// file flags
#ifndef _SPIPRIfl
#define _SPIPRIfl  "%02x"           /* Flag */
#endif

/*********************************************************************************************************
  ���������� ���ֹ�ֵ
  Reference:
    Garbage collecting examines all pages in a block which and sums up
to a block score. Deleted pages normally gives positive score and
used pages normally gives a negative score (as these must be moved).
To have a fair wear-leveling, the erase age is also included in score,
whose factor normally is the most positive.
The larger the score, the more likely it is that the block will
picked for garbage collection.
*********************************************************************************************************/
// Garbage collecting heuristics - weight used for deleted pages.
#ifndef SPIFFS_GC_HEUR_W_DELET
#define SPIFFS_GC_HEUR_W_DELET          (5)             /* ��ɾ��ҳ���ṩ5�� */
#endif
// Garbage collecting heuristics - weight used for used pages.
#ifndef SPIFFS_GC_HEUR_W_USED
#define SPIFFS_GC_HEUR_W_USED           (-1)            /* �ѱ�ռ�õ�Ҳ�����ṩ��-1�� */
#endif
// Garbage collecting heuristics - weight used for time between
// last erased and erase of this block.
#ifndef SPIFFS_GC_HEUR_W_ERASE_AGE
#define SPIFFS_GC_HEUR_W_ERASE_AGE      (50)            /* �����ṩ50�� */
#endif

/*********************************************************************************************************
  Object������󳤶ȣ�ע�⣺�����ַ������� = SPIFFS_OBJ_NAME_LEN - 1 = 31
*********************************************************************************************************/
#ifndef SPIFFS_OBJ_NAME_LEN
#define SPIFFS_OBJ_NAME_LEN             (32)
#endif
/*********************************************************************************************************
  �ɷ���Buffer��С��ԽС��ζ�Ÿ���Ķ�д��������������ζ������������߼�ҳ���С
  Reference:
    Size of buffer allocated on stack used when copying data.
Lower value generates more read/writes. No meaning having it bigger
than logical page size.
*********************************************************************************************************/
#ifndef SPIFFS_COPY_BUFFER_STACK
#define SPIFFS_COPY_BUFFER_STACK        (64)
#endif

/*********************************************************************************************************
  �豸����
  ��SylixOS�����ǽ���ֲSingleton
*********************************************************************************************************/
#ifndef SPIFFS_CFG_PHYS_SZ
#define SPIFFS_CFG_PHYS_SZ(ignore)        (1024 * 1024 * 2)
#endif
#ifndef SPIFFS_CFG_PHYS_ERASE_SZ
#define SPIFFS_CFG_PHYS_ERASE_SZ(ignore)  (65536)
#endif
#ifndef SPIFFS_CFG_PHYS_ADDR
#define SPIFFS_CFG_PHYS_ADDR(ignore)      (0)
#endif
#ifndef SPIFFS_CFG_LOGIC_PAGE_SZ
#define SPIFFS_CFG_LOGIC_PAGE_SZ(ignore)    (256)
#endif
#ifndef SPIFFS_CFG_LOGIC_BLOCK_SZ
#define SPIFFS_CFG_LOGIC_BLOCK_SZ(ignore)   (65536)
#endif
/*********************************************************************************************************
  �Ƿ��������õ��ļ����棿
  Reference:
    Enable this to add a temporal file cache using the fd buffer.
The effects of the cache is that SPIFFS_open will find the file faster in
certain cases. It will make it a lot easier for spiffs to find files
opened frequently, reducing number of readings from the spi flash for
finding those files.
This will grow each fd by 6 bytes. If your files are opened in patterns
with a degree of temporal locality, the system is optimized.
Examples can be letting spiffs serve web content, where one file is the css.
The css is accessed for each html file that is opened, meaning it is
accessed almost every second time a file is opened. Another example could be
a log file that is often opened, written, and closed.
The size of the cache is number of given file descriptors, as it piggybacks
on the fd update mechanism. The cache lives in the closed file descriptors.
When closed, the fd know the whereabouts of the file. Instead of forgetting
this, the temporal cache will keep handling updates to that file even if the
fd is closed. If the file is opened again, the location of the file is found
directly. If all available descriptors become opened, all cache memory is
lost.
�����ҵ�Ԥ����˼��һ���𣿣�
*********************************************************************************************************/
#ifndef SPIFFS_TEMPORAL_FD_CACHE
#define SPIFFS_TEMPORAL_FD_CACHE              1
#endif

/*********************************************************************************************************
  ���е÷֣�Ѱ������ļ� =-=���ⲻ���һ����
  Reference:
    Temporal file cache hit score. Each time a file is opened, all cached files
will lose one point. If the opened file is found in cache, that entry will
gain SPIFFS_TEMPORAL_CACHE_HIT_SCORE points. One can experiment with this
value for the specific access patterns of the application. However, it must
be between 1 (no gain for hitting a cached entry often) and 255.
be between 1 (no gain for hitting a cached entry often) and 255.
*********************************************************************************************************/
#ifndef SPIFFS_TEMPORAL_CACHE_HIT_SCORE
#define SPIFFS_TEMPORAL_CACHE_HIT_SCORE       4
#endif

/*********************************************************************************************************
  �Ƿ���������Indexҳ��ӳ�䵽�ڴ���
  Reference:
    This allows for faster and more deterministic reading if cases of reading
large files and when changing file offset by seeking around a lot.
When mapping a file's index, the file system will be scanned for index pages
and the info will be put in memory provided by user. When reading, the
memory map can be looked up instead of searching for index pages on the
medium. This way, user can trade memory against performance.
Whole, parts of, or future parts not being written yet can be mapped. The
memory array will be owned by spiffs and updated accordingly during garbage
collecting or when modifying the indices. The latter is invoked by when the
file is modified in some way. The index buffer is tied to the file
descriptor.
*********************************************************************************************************/
#ifndef SPIFFS_EN_IX_MAP
#define SPIFFS_EN_IX_MAP                         0
#endif

/*********************************************************************************************************
  һЩ���ӻ�����
*********************************************************************************************************/
#ifndef spiffs_printf
#define spiffs_printf(...)                printf(__VA_ARGS__)
#endif
// spiffs_printf argument for a free page
#ifndef SPIFFS_TEST_VIS_FREE_STR
#define SPIFFS_TEST_VIS_FREE_STR          "_"       /* ����FreePage */
#endif
// spiffs_printf argument for a deleted page
#ifndef SPIFFS_TEST_VIS_DELE_STR
#define SPIFFS_TEST_VIS_DELE_STR          "/"       /* ����DeletedPage */
#endif
// spiffs_printf argument for an index page for given object id
#ifndef SPIFFS_TEST_VIS_INDX_STR
#define SPIFFS_TEST_VIS_INDX_STR(id)      "i"       /* ����IndexPage */
#endif
// spiffs_printf argument for a data page for given object id
#ifndef SPIFFS_TEST_VIS_DATA_STR
#define SPIFFS_TEST_VIS_DATA_STR(id)      "d"       /* ����DataPage */
#endif

#define SPIFFS_GC_DBG
#define SPIFFS_DBG
#define SPIFFS_CHECK_DBG
#define SPIFFS_API_DBG


#ifndef SPIFFS_DBG
#define SPIFFS_DBG(_f, ...)   printf(_f, ## __VA_ARGS__)
#endif
// Set spiffs debug output call for garbage collecting.
#ifndef SPIFFS_GC_DBG
#define SPIFFS_GC_DBG(_f, ...) printf(_f, ## __VA_ARGS__)
#endif
// Set spiffs debug output call for caching.
#define SPIFFS_CACHE_DBG

#ifndef SPIFFS_CACHE_DBG
#define SPIFFS_CACHE_DBG(_f, ...) printf(_f, ## __VA_ARGS__)
#endif
// Set spiffs debug output call for system consistency checks.
#ifndef SPIFFS_CHECK_DBG
#define SPIFFS_CHECK_DBG(_f, ...) printf(_f, ## __VA_ARGS__)
#endif
// Set spiffs debug output call for all api invocations.
#ifndef SPIFFS_API_DBG
#define SPIFFS_API_DBG(_f, ...) printf(_f, ## __VA_ARGS__)
#endif

/*********************************************************************************************************
  SPIFFS������������
*********************************************************************************************************/
typedef UINT16 SPIFFS_BLOCK_IX;                     /* Block Index���� */
typedef UINT16 SPIFFS_PAGE_IX;                      /* Page  Index���� */
typedef UINT16 SPIFFS_OBJ_ID;                       /* Object   ID���� */
typedef UINT16 SPIFFS_SPAN_IX;                      /* Span  Index���� */

#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFFSCONFIG_H_ */