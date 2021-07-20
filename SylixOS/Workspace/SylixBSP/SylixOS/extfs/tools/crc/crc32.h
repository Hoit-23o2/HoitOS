/*
 * list_template.h
 *
 *  Created on: Jun 2, 2021
 *      Author: Administrator
 */

#ifndef SYLIXOS_EXTFS_TOOLS_CRC_H_
#define SYLIXOS_EXTFS_TOOLS_CRC_H_

#include "SylixOS.h"

#define CRCPOLY_LE 0xedb88320

static UINT32 crc32_le(PUCHAR p, UINT len)
{
	INT i;
	UINT32 crc;
	while (len--) {
		crc ^= *p++;
		for (i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
	}
	return crc;
}

#endif /* SYLIXOS_EXTFS_TOOLS_CRC_H_ */
