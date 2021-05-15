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
** ��   ��   ��: symbol.h								
**														
** ��   ��   ��: makesymbol ����						
**														
** �ļ���������: 2021 �� 05 �� 15 ��		
**														
** ��        ��: ϵͳ sylixos ���ű�. (���ļ��� makesymbol �����Զ�����, �����޸�)	
*********************************************************************************************************/	
														
#ifndef __SYMBOL_H										
#define __SYMBOL_H										
														
#include "SylixOS.h"									
#include "symboltools.h"								
														
#ifdef SYLIXOS_EXPORT_KSYMBOL							
#define SYM_TABLE_SIZE 7251							
extern  LW_STATIC_SYMBOL  _G_symLibSylixOS[SYM_TABLE_SIZE];					
															
static LW_INLINE  INT symbolAddAll (VOID)				
{														
    return  (symbolAddStatic((LW_SYMBOL *)_G_symLibSylixOS, SYM_TABLE_SIZE));	
}														
#else													
static LW_INLINE  INT symbolAddAll (VOID)				
{														
    return  (ERROR_NONE);								
}														
#endif													
														
#endif                                                                  /*  __SYMBOL_H                  */	
/*********************************************************************************************************	
  END													
*********************************************************************************************************/	
