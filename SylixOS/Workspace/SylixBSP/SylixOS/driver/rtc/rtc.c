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
** ��   ��   ��: rtc.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 09 �� 20 ��
**
** ��        ��: RTC ����
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"                                                     /*  �������� & ���������       */
#include "SylixOS.h"
#include "time.h"
/*********************************************************************************************************
** Function name:           rtcSetTime
** Descriptions:            ���� RTC ʱ��
** input parameters:        prtcTimeNow       ��ǰʱ��
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/12/20
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static int  __rtcSetTime (PLW_RTC_FUNCS  prtcfuncs, time_t  *ptimeNow)
{
    INTREG       iregInterLevel;
    struct tm    tmNow;
    
    int     iHigh;                                                      /*  BCD ���λ                  */
    int     iLow;                                                       /*  BCD ���λ                  */
    
    gmtime_r(ptimeNow, &tmNow);                                         /*  ת���� tm ʱ���ʽ          */
    
    API_InterLock(&iregInterLevel);
    
    rRTCCON = (unsigned char)((rRTCCON & ~(0xf)) | 0x1);
    
    iHigh   = tmNow.tm_sec / 10;
    iLow    = tmNow.tm_sec % 10;
    rBCDSEC = (unsigned char)((iHigh << 4) + iLow);                     /*  ��Ĵ���                    */
    
    iHigh   = tmNow.tm_min / 10;
    iLow    = tmNow.tm_min % 10;
    rBCDMIN = (unsigned char)((iHigh << 4) + iLow);                     /*  �ּĴ���                    */
    
    iHigh    = tmNow.tm_hour / 10;
    iLow     = tmNow.tm_hour % 10;
    rBCDHOUR = (unsigned char)((iHigh << 4) + iLow);                    /*  Сʱ�Ĵ���                  */
    
    iHigh    = tmNow.tm_mday / 10;
    iLow     = tmNow.tm_mday % 10;
    rBCDDATE = (unsigned char)((iHigh << 4) + iLow);                    /*  ���ڼĴ���                  */
    
    iHigh    = (tmNow.tm_mon + 1) / 10;
    iLow     = (tmNow.tm_mon + 1) % 10;
    rBCDMON  = (unsigned char)((iHigh << 4) + iLow);                    /*  �·ݼĴ���                  */
    
    iHigh    = (tmNow.tm_year + 1900 - 2000) / 10;
    iLow     = (tmNow.tm_year + 1900 - 2000) % 10;
    rBCDYEAR = (unsigned char)((iHigh << 4) + iLow);                    /*  ��Ĵ���                    */

    rBCDDAY  = (unsigned char)(tmNow.tm_wday + 1);                      /*  ���ڼĴ���   2440: 1~7      */

    rRTCCON  = 0x0;

    API_InterUnlock(iregInterLevel);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** Function name:           rtcGetTime
** Descriptions:            ��ȡ RTC ʱ��
** input parameters:        NONE
** output parameters:       prtcTimeNow       ��ǰʱ��
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/12/20
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static int  __rtcGetTime (PLW_RTC_FUNCS  prtcfuncs, time_t  *ptimeNow)
{
    struct tm    tmNow;
    
    int     iHigh;                                                      /*  BCD ���λ                  */
    int     iLow;                                                       /*  BCD ���λ                  */
    
    rRTCCON = (unsigned char)((rRTCCON & ~(0xf)) | 0x1);
    
    iHigh = ((rBCDSEC & 0xF0) >> 4);
    iLow  =  (rBCDSEC & 0x0F);
    tmNow.tm_sec = (unsigned char)(iHigh * 10 + iLow);
    
    iHigh = ((rBCDMIN & 0xF0) >> 4);
    iLow  =  (rBCDMIN & 0x0F);
    tmNow.tm_min = (unsigned char)(iHigh * 10 + iLow);
    
    iHigh = ((rBCDHOUR & 0xF0) >> 4);
    iLow  =  (rBCDHOUR & 0x0F);
    tmNow.tm_hour = (unsigned char)(iHigh * 10 + iLow);
    
    iHigh = ((rBCDDATE & 0xF0) >> 4);
    iLow  =  (rBCDDATE & 0x0F);
    tmNow.tm_mday = (unsigned char)(iHigh * 10 + iLow);
    
    iHigh = ((rBCDMON & 0xF0) >> 4);
    iLow  =  (rBCDMON & 0x0F);
    tmNow.tm_mon = (unsigned char)((iHigh * 10 + iLow) - 1);
    
    iHigh = ((rBCDYEAR & 0xF0) >> 4);
    iLow  =  (rBCDYEAR & 0x0F);
    tmNow.tm_year = (unsigned short)(iHigh * 10 + iLow + 2000 - 1900);
    
    tmNow.tm_wday = (unsigned char)(rBCDDAY - 1);
    
    rRTCCON  = 0x0;
    
    if (ptimeNow) {
        *ptimeNow = timegm(&tmNow);
    }
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** Function name:           rtcGetFuncs
** Descriptions:            ��ȡ RTC ��������
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          pfuncs      ��������
** Created by:              Hanhui
** Created Date:            2007/12/20
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
PLW_RTC_FUNCS   rtcGetFuncs (VOID)
{
    static LW_RTC_FUNCS    rtcfuncs = {LW_NULL, __rtcSetTime, __rtcGetTime, LW_NULL};
    
    return  (&rtcfuncs);
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/
