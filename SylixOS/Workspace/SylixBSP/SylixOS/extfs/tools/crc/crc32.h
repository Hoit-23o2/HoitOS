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

static UINT32 crc32_le(unsigned char* p, UINT len)
{
	INT i;
	UINT32 crc = 0;
	PCHAR originPC = ((char*)p)+sizeof(HOIT_RAW_INODE);
	PCHAR originP = (char*)p;
	while (len--) {
		crc ^= *p++;
		for (i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
	}

	if(crc == 2711013869){
	    snprintf("%s \n", 925, originPC);
	    printf("yes\n");
	}

	return crc;
}

#endif /* SYLIXOS_EXTFS_TOOLS_CRC_H_ */
