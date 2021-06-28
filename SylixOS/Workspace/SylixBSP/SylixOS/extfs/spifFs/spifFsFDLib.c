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
** 文   件   名: spifFsFDLib.h
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 10日
**
** 描        述: Spiffs文件句柄管理库
*********************************************************************************************************/

#include "spifFsFDLib.h"
#include "spifFsLib.h"
#include "spifFsCache.h"

INT32 spiffsFdGet(PSPIFFS_VOLUME pfs, SPIFFS_FILE file, PSPIFFS_FD *pfd) {
		if (file <= 0 || file > (INT16)pfs->uiFdCount) {
			return SPIFFS_ERR_BAD_DESCRIPTOR;
		}
		PSPIFFS_FD pFds = (PSPIFFS_FD)pfs->pucFdSpace;
		*pfd = &pFds[file - 1];
		if ((*pfd)->fileN == 0) {
			return SPIFFS_ERR_FILE_CLOSED;
		}
		return SPIFFS_OK;
}


/*********************************************************************************************************
** 函数名称: spiffsCBObjectEvent
** 功能描述: 用于注册事件回调
** 输　入  : pfs          文件头
**           pObjId        返回的Object ID
**           pucConflictingName 文件路径名
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID spiffsCBObjectEvent(PSPIFFS_VOLUME pfs, PSPIFFS_PAGE_OBJECT_IX objIX, INT ev,
												 SPIFFS_OBJ_ID objIdRaw, SPIFFS_SPAN_IX spanIX, SPIFFS_PAGE_IX pageIXNew,
												 UINT uiNewSize){
		(VOID)objIX;
		// update index caches in all file descriptors
		SPIFFS_OBJ_ID objId = objIdRaw & ~SPIFFS_OBJ_ID_IX_FLAG;
		UINT i;
		PSPIFFS_FD pFds = (PSPIFFS_FD)pfs->pucFdSpace;
		PSPIFFS_FD pFdCur;
		UINT32 uiActNewSize;
		SPIFFS_DBG("       CALLBACK  %s obj_id:"_SPIPRIid" spix:"_SPIPRIsp" npix:"_SPIPRIpg" nsz:"_SPIPRIi"\n", 
							 (const char *[]){"UPD", "NEW", "DEL", "MOV", "HUP","???"}[MIN(ev, 5)],
								objIdRaw, spanIX, pageIXNew, uiNewSize);
		/* 更新所有fd */
		for (i = 0; i < pfs->uiFdCount; i++) {
				pFdCur = &pFds[i];
				if ((pFdCur->objId & ~SPIFFS_OBJ_ID_IX_FLAG) != objId) 	/* 该文件描述符与被更新的文件无关 */
					continue; // fd not related to updated file

				if (spanIX == 0) { // 更新Index Heaader
						if (ev != SPIFFS_EV_IX_DEL) {	/* 如果不是删除一个Object Index */
								SPIFFS_DBG("       callback: setting fd "_SPIPRIfd":"_SPIPRIid"(fdoffs:"_SPIPRIi" offs:"_SPIPRIi") objix_hdr_pix to "_SPIPRIpg", size:"_SPIPRIi"\n",
												   (pFdCur->fileN), pFdCur->objId, pFdCur->uiFdOffset, pFdCur->uiOffset, pageIXNew, uiNewSize);
								pFdCur->pageIXObjIXHdr = pageIXNew;
								if (uiNewSize != 0) {
										// update size and offsets for fds to this file
										pFdCur->uiSize = uiNewSize;
										uiActNewSize = uiNewSize == SPIFFS_UNDEFINED_LEN ? 0 : uiNewSize;
										if (uiActNewSize > 0 && pFdCur->pCachePage) {
												uiActNewSize = MAX(uiActNewSize, pFdCur->pCachePage->uiOffset + pFdCur->pCachePage->uiSize);
										}
										if (pFdCur->uiOffset > uiActNewSize) {
												pFdCur->uiOffset = uiActNewSize;
										}
										if (pFdCur->uiFdOffset > uiActNewSize) {
												pFdCur->uiFdOffset = uiActNewSize;
										}
										/* 文件被截断了 */
										if (pFdCur->pCachePage && pFdCur->pCachePage->uiOffset > uiActNewSize + 1) {
												SPIFFS_CACHE_DBG("CACHE_DROP: file trunced, dropping cache page "_SPIPRIi", no writeback\n", 
																				 pFdCur->pCachePage->uiIX);
												spiffsCacheFdRelease(pfs, pFdCur->pCachePage);
										}
								}
						} 
						else {
								// removing file
								if (pFdCur->fileN && pFdCur->pCachePage) {
										SPIFFS_CACHE_DBG("CACHE_DROP: file deleted, dropping cache page "_SPIPRIi", no writeback\n", 
																		 pFdCur->pCachePage->uiIX);
										spiffsCacheFdRelease(pfs, pFdCur->pCachePage);
								}
								SPIFFS_DBG("       callback: release fd "_SPIPRIfd":"_SPIPRIid" span:"_SPIPRIsp" objix_pix to "_SPIPRIpg"\n", 
													 pFdCur->fileN, pFdCur->objId, spanIX, pageIXNew);
								pFdCur->fileN = 0;
								pFdCur->objId = SPIFFS_OBJ_ID_DELETED;
						}
				} 
				if (pFdCur->spanIXObjIXCursor == spanIX) {	/* 更新当前Page的指针 */
						if (ev != SPIFFS_EV_IX_DEL) {
								SPIFFS_DBG("       callback: setting fd "_SPIPRIfd":"_SPIPRIid" span:"_SPIPRIsp" objix_pix to "_SPIPRIpg"\n", 
													 (pFdCur->fileN), pFdCur->objId, spanIX, pageIXNew);
								pFdCur->pageIXObjIXCursor = pageIXNew;
						} else {
								pFdCur->pageIXObjIXCursor = 0;
						}
				}
		} 

		//TODO: Index Map
		// // update index maps
		// if (ev == SPIFFS_EV_IX_UPD || ev == SPIFFS_EV_IX_NEW) {
		// 		for (i = 0; i < pfs->uiFdCount; i++) {
		// 				pFdCur = &pFds[i];
		// 				// check fd opened, having ix map, match obj id
		// 				if (pFdCur->fileN == 0 ||
		// 						pFdCur->pIXMap == LW_NULL ||
		// 						(pFdCur->objId & ~SPIFFS_OBJ_ID_IX_FLAG) != objId) 
		// 						continue;
		// 				SPIFFS_DBG("       callback: map ix update fd "_SPIPRIfd":"_SPIPRIid" span:"_SPIPRIsp"\n", 
		// 									 (pFdCur->fileN), pFdCur->objId, spanIX);
		// 				spiffs_update_ix_map(pfs, pFdCur, spanIX, objIX);
		// 		}
		// }

		SPIFFS_FILEOP_TYPE op;
		// callback to user if object index header
		if (pfs->fileCallbackFunc && 
				spanIX == 0 && 
				(objIdRaw & SPIFFS_OBJ_ID_IX_FLAG)) {
				if (ev == SPIFFS_EV_IX_NEW) {
						op = SPIFFS_CB_CREATED;
				} 
				else if (ev == SPIFFS_EV_IX_UPD || ev == SPIFFS_EV_IX_MOV || ev == SPIFFS_EV_IX_UPD_HDR) {
						op = SPIFFS_CB_UPDATED;
				} 
				else if (ev == SPIFFS_EV_IX_DEL) {
						op = SPIFFS_CB_DELETED;
				} 
				else {
						SPIFFS_DBG("       callback: WARNING unknown callback event "_SPIPRIi"\n", ev);
						return; // bail out
				}
				pfs->fileCallbackFunc(pfs, op, objId, pageIXNew);
		}
}