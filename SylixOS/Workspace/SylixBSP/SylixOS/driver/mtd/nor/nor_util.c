/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: nor_util.c
**
** ��   ��   ��: Pan yanqi (������)
**
** �ļ���������: 2021 �� 03 �� 11 ��
**
** ��        ��: NorFlash����������߿�
*********************************************************************************************************/
#include "nor.h"
#include "nor_util.h"
#include "fake_nor.h"

/*********************************************************************************************************
** ��������: write_word_to_mem
** ��������: �������ڴ�дoffset��ַдData
** �䡡��  : base         norflash��ʼ��ַ
**			 offset		  Ƭ��ƫ��
**			 data		  д�������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID write_word_to_mem(UINT32 base, UINT32 offset, UINT16 data)
{
	volatile UINT16* p = (volatile UINT16*)(base + (offset << 1));
	*p = data;
}
/*********************************************************************************************************
** ��������: read_word_from_mem
** ��������: �������ڴ�offset����һ����
** �䡡��  : base         norflash��ʼ��ַ
**			 offset		  Ƭ��ƫ��
** �䡡��  : 16λ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
UINT16 read_word_from_mem(UINT32 base, UINT32 offset)
{
	volatile UINT16* p = (volatile UINT16*)(base + (offset << 1));
	return *p;
}
/*********************************************************************************************************
** ��������: read_word_from_mem
** ��������: �������ڴ�offset����һ���ֽ�
** �䡡��  : base         norflash��ʼ��ַ
**			 offset		  Ƭ��ƫ��
** �䡡��  : 16λ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
UINT8 read_byte_from_mem(UINT32 base, UINT32 offset){
	volatile UINT8* p = (volatile UINT8*)(base + offset);
	return *p;
}

/*********************************************************************************************************
** ��������: wait_ready
** ��������: �ȴ���д�������
** �䡡��  : base         norflash��ʼ��ַ
**			 offset		  Ƭ��ƫ��
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID wait_ready(UINT32 base, UINT32 offset)
{
	if(IS_FAKE_MODE()){
		return;
	}
	else{
		UINT32 val;
		UINT32 pre;

		pre = read_word_from_mem(base, offset >> 1);
		val = read_word_from_mem(base, offset >> 1);
		while ((val & (1 << 6)) != (pre & (1 << 6)))			/* �Ƚ� Q6 λ */
		{
#ifdef NOR_DEBUG
		// printf("pre: %d, val: %d\n", (val & (1 << 6)), (pre & (1 << 6)));
		// printf("pre: %d, val: %d\n", val, pre);
		// printf("pre: %u, val: %u\n", val, pre);
		// printf("pre: %x, val: %x\n", val, pre);
#endif  // NOR_DEBUG
			pre = val;
			val = read_word_from_mem(base, offset >> 1);		
		}
	}
	
}

/*********************************************************************************************************
** ��������: nor_command_unlock
** ��������: ���ڽ���norflash��д�����豸ID������ID
** �䡡��  : base         norflash��ʼ��ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID nor_command_unlock(UINT32 base){
	if(IS_FAKE_MODE()){
		return;
	}
	else{
		write_word_to_mem(base, 0x555, 0xAA); 			/* ���� */
		write_word_to_mem(base, 0x2AA, 0x55); 
	}
}
/*********************************************************************************************************
** ��������: nor_reset
** ��������: ��������NorFlash����
** �䡡��  : base         norflash��ʼ��ַ
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID nor_reset(UINT32 base){
	if(IS_FAKE_MODE()){
		lib_memset((PVOID)NOR_FLASH_BASE, (INT)-1, NOR_FLASH_SZ);
		lib_memset((PVOID)sector_infos, 0, NOR_FLASH_NSECTOR * sizeof(sector_info_t));
		INT i;
		for (i = 0; i < NOR_FLASH_NSECTOR; i++)
		{
			sector_infos[i].erase_cnt = 0;
			sector_infos[i].is_bad = FALSE;
		}
	}
	else{
		write_word_to_mem(base, 0x0, 0xF0);	
	}
}

/*********************************************************************************************************
** ��������: nor_check_offset_range
** ��������: �鿴offset�Ƿ�λ��оƬ��
** �䡡��  : base         norflash��ʼ��ַ
** 			 offset		  Ƭ��ƫ��
**			 size_bytes   ��д�������Ĵ�С 
** �䡡��  : TRUE or False
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL nor_check_offset_range(UINT32 base, UINT32 offset, UINT size_bytes){
	if(offset + size_bytes > NOR_FLASH_SZ || offset < base){
		return FALSE;
	}
	return TRUE;
}
/*********************************************************************************************************
** ��������: nor_check_modifiable_perm
** ��������: �鿴offset�Ƿ�Ϸ�����Ϊǰ226KB��Uboot���򣬲���д��������
** �䡡��  : offset		  Ƭ��ƫ��
** �䡡��  : ���Խ����޸�TRUE�� ���ɽ����޸�False
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL nor_check_modifiable_perm(UINT32 offset){
	if(offset < NOR_FLASH_START_OFFSET){
		return FALSE;
	}
	return TRUE;
}
/*********************************************************************************************************
** ��������: nor_check_should_erase
** ��������: �鿴����д������Ƿ���Ҫ����
** ԭ����  : ����ԭ�� NorFlash�ɽ�1ֱ�ӱ��Ϊ0
**			a1...a8����0��1
**			����Ҫд���ֽ�byte_to_write�� a1  a2  a3  ... a8
**			�����������ֽ�byte_in_flash�� a1' a2' a3' ... a8'
**			������֮�������byte_diff�� (a1 ^ a1') (a2 ^ a2') ... (a8 ^ a8')
**			
**			��ˣ����Ǳ��ܹ��ҵ� ��Ҫд�� �� Flash�ϵ��ֽڵĲ�ͬλ���ڵ�ȫ���ҳ������Ҳ�ͬλ��Ϊ1
**			�����Ҫд�ľ��Ǵ�1дΪ0����ôbyte_diff��byte_to_write��Ľ��һ����0
** �䡡��  : offset		  Ƭ��ƫ��
**			 content	  д������
**			 size_bytes	  д���ֽڴ�С
** �䡡��  : ��Ҫ����TRUE ������Ҫ����False
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL nor_check_should_erase(UINT32 base, UINT offset, PCHAR content, UINT size_bytes){
	INT i;
	for (i = 0; i < size_bytes; i++)
	{
		UINT8 byte_in_flash = read_byte_from_mem(base, offset + i);
		UINT8 byte_to_write = (UINT8)(*(content + i));
		UINT8 byte_diff = byte_in_flash ^ byte_to_write;
#ifdef NOR_DEBUG
		if((byte_diff & byte_to_write) != 0){
			printf("byte_in_flash: %x\n", byte_in_flash);
			printf("byte_to_write: %x\n", byte_to_write);
			printf("byte_diff: %x\n", byte_diff);
			printf("byte_diff & byte_to_write: %d \n", byte_diff & byte_to_write);
		}
#endif // NOR_DEBUG
		if((byte_diff & byte_to_write) != 0){
			return TRUE;
		}
	}
	return FALSE;
}

/*********************************************************************************************************
** ��������: nor_write_buffer
** ��������: ��buffer�е�����д��norflash�У�ֻ����д�벻���������sector��offset����sectorʣ��ռ�����ݣ�
** �䡡��  : base         norflash��ʼ��ַ
** 			 offset		  Ƭ��ƫ��
**			 content      д�������
**			 size_bytes   ��д��Ĵ�С 
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID nor_write_buffer(UINT32 base, UINT offset, PCHAR _content, UINT size_bytes){
	UINT8 sector_no = GET_SECTOR_NO(offset);
	UINT32 sector_start_offset = GET_SECTOR_OFFSET(sector_no);
	UINT sector_remain_size = GET_SECTOR_SIZE(sector_no) - (offset - sector_start_offset); 
	PUCHAR content = (PUCHAR )_content;
	if(size_bytes > sector_remain_size){
#ifdef NOR_DEBUG
		pretty_print("[nor write buffer]", FAIL "size not permit", DONT_CENTRAL);
		printf("size_bytes: %d\n", size_bytes);
		printf("sector_remain_size: %d\n", sector_remain_size);
		printf("offset: %d\n", offset);
#endif // NOR_DEBUG
		return;
	}
	if(IS_FAKE_MODE()){
		PCHAR p = content;
		INT	  i, size_words;
		volatile INT fake;
		if(get_sector_is_bad(sector_no)){                                                          /* ����޸� */
#ifdef NOR_DEBUG
			printf("| -> [sector #%d is bad, there may be some error(s), remember to check]\n", sector_no);
#endif // DEBUG
			PCHAR pe = p + size_bytes;
			for (; p < pe; p++)
			{
				INT possibily = rand() % 100 + 1;
				INT random_change = rand() % 127;                                          			/* 0 ~ 127 ascii */
				if(possibily >= 50){                                                       			/* 50%�ļ���д�� */
					*p += random_change;
				}
			} 
		}
		size_words = size_bytes / 2;
		for (i = 0; i < size_words; i++)
		{
			/* ģ��nor_cmd_unlock�� 2���ô����� */
			fake = 1;		/* Cycle 1 */
			fake = 2;		/* Cycle 2 */
			/* ģ��д�룬2���ô����� */
			fake = 3;		/* Cycle 3 */
			fake = 4;		/* Cycle 4 */
			/* ģ��wait_ready�� ����2���ô����� */
			fake = 5;		/* Cycle 5 */
			fake = 6;		/* Cycle 6 */
		}
		lib_memcpy((PVOID)(base + offset), content, size_bytes);
	}
	else {
		UINT size_words = size_bytes / 2;
		INT remain_byte = size_bytes - size_words * 2 ;
		INT i;
#ifdef NOR_DEBUG
		printf("size_words: %d, remain_byte: %dB \n", size_words, remain_byte);
#endif // NOR_DEBUG
		for (i = 0; i < size_words; i++)
		{
			INT index = 2 * i;
			UINT16 data = content[index] + (content[index + 1] << 8);
			nor_command_unlock(base);
			write_word_to_mem(base, 0x555, 0xA0);
			write_word_to_mem(base, offset >> 1, data);
			wait_ready(base, offset);	   
			offset += 2;
		}
		if(remain_byte){
			UINT16 data = content[size_bytes - 1];
			nor_command_unlock(base);
			write_word_to_mem(base, 0x555, 0xA0);
			write_word_to_mem(base, offset >> 1, data);
			wait_ready(base, offset);	   
		}
	}
}

/*********************************************************************************************************
** ��������: nor_erase_sector
** ��������: ����offset����һ������
** �䡡��  : base		  norflash��ʼ��ַ
** 			 offset		  ƫ�Ƶ�ַ		  
** �䡡��  : 0 �ɹ� , -1ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID nor_erase_sector(UINT32 base, UINT offset){
	if(IS_FAKE_MODE()){
		UINT8 sector_no = GET_SECTOR_NO(offset);
		UINT32 sector_start_offset = GET_SECTOR_OFFSET(sector_no);
		UINT sector_size = GET_SECTOR_SIZE(sector_no);
		lib_memset((PVOID)(base + sector_start_offset), (INT)-1, sector_size);
		sector_infos[sector_no].erase_cnt++;
	}
	else{
		nor_command_unlock(NOR_FLASH_BASE);
		write_word_to_mem(NOR_FLASH_BASE,0x555, 0x80);	 
		nor_command_unlock(NOR_FLASH_BASE);
		write_word_to_mem(NOR_FLASH_BASE,offset >> 1, 0x30);	 	           /* ����������ַ */ 
		wait_ready(NOR_FLASH_BASE, offset);									   /* �ȴ�������� */
	}
}
/*********************************************************************************************************
** ��������: nor_erase_range
** ��������: ��[low_sector_no, high_sector_no)һ������
** �䡡��  : low_sector_no		  ��sector��
** 			 high_sector_no		  ��sector��
**			 erase_nor      	  ������������
** �䡡��  : 0 �ɹ� , -1ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
UINT8 nor_erase_range(UINT8 low_sector_no, UINT8 high_sector_no, UINT8 (*erase_nor)(UINT, ENUM_ERASE_OPTIONS)){
	INT i;
	for (i = low_sector_no; i < high_sector_no; i++)
	{
		if(erase_nor(GET_SECTOR_OFFSET(i), ERASE_SECTOR) < 0){
#ifdef NOR_DEBUG
			CHAR temp[TEMP_BUF_SZ];
			lib_memset(temp, 0, TEMP_BUF_SZ);
			sprintf(temp, WARN "try to erase protected sector %d", i);
			pretty_print("[nor erase range statue]", temp, DONT_CENTRAL);
#endif // DEBUG
		}
	}
	return 0;
}
/*********************************************************************************************************
** ��������: nor_erase_region
** ��������: ��װ����Region���֣�����erase_nor�ĵ���
** �䡡��  : region_no         �ײ�ƫ��
**           erase_nor         ��������
** �䡡��  : 0�ɹ��� -1ʧ��
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
UINT8 nor_erase_region(INT8 region_no, UINT8 (*erase_nor)(UINT, ENUM_ERASE_OPTIONS)){
	INT count;
	INT region_base;
	switch (region_no)
	{
	case 0:
		count = 1;
		region_base = 0;
		break;
	case 1:
		count = 2;
		region_base = 1;
		break;
	case 2:
		count = 1;
		region_base = 3;
		break;
	case 3:
		count = 31;
		region_base = 4;
		break;
	default:
		break;
	}
	if(nor_erase_range(region_base, region_base + count, erase_nor) < 0){
		// return -1
#ifdef NOR_DEBUG
		CHAR temp[TEMP_BUF_SZ];
		lib_memset(temp, 0, TEMP_BUF_SZ);
		sprintf(temp, WARN "region %d has protected sectors", region_no);
		pretty_print("[nor erase region statue]", temp, DONT_CENTRAL);
#endif // DEBUG
	}
	return 0;
}

/*********************************************************************************************************
** ��������: pretty_print
** ��������: ������ӡ��ʽ
** �䡡��  : header         �ײ�����
**           content          ����
**			 centralized	  �Ƿ���д�ӡ��������д�ӡ����ֻ��ӡheader������
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID pretty_print(PCHAR header, PCHAR content, BOOL centralized){
    INT space_i;
    INT width = NMAX_DISPLAY;
	if(!centralized){
		printf("| %s", header);
		INT header_len = lib_strlen(header) + 2;
		INT content_len = lib_strlen(content) + 2;
		INT space_len = width - header_len - content_len;
		for (space_i = 0; space_i < space_len; space_i++)
		{
			printf(" ");
		}
		printf("%s |\n", content);
	}
    else
	{
		INT header_len = lib_strlen(header) + 4;
		INT l_minus_len = (width - header_len) / 2;
		INT r_minus_len = width - header_len - l_minus_len;
		printf("| ");
		for (space_i = 1; space_i < l_minus_len; space_i++)
		{
			printf(" ");
		}
		printf("%s", header);
		for (space_i = 0; space_i < r_minus_len - 1; space_i++)
		{
			printf(" ");
		}
		printf(" |\n");
	}
	
}
/*********************************************************************************************************
** ��������: show_divider
** ��������: ��ʾ�ָ���
** �䡡��  : header         ������-----header-----
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID show_divider(PCHAR header){
	INT i;
	INT width = NMAX_DISPLAY;
	printf("+");
	if(header == LW_NULL){
		for (i = 1; i < width - 1; i++)
		{
			printf("-");
		}	
	}
	else
	{
		INT header_len = lib_strlen(header);
		INT l_minus_len = (width - header_len / 2);
		INT r_minus_len = width - header_len - l_minus_len;
		for (i = 1; i < l_minus_len; i++)
		{
			printf("-");
		}
		printf("%s", header);
		for (i = 0; i < r_minus_len - 1; i++)
		{
			printf("-");
		}
	}
	printf("+\n");
}


