/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: timer.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 12 �� 09 ��
**
** ��        ��: S3C2440 Ӳ����ʱ��.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "timer.h"
/*********************************************************************************************************
** Function name:           timerSetPrescaler0
** Descriptions:            ���ö�ʱ��0 �� ��ʱ��1 ��Ԥ��Ƶ����
** input parameters:        ucPrescaler     �µ�Ԥ��Ƶ����
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
void  timerSetPrescaler0 (unsigned char   ucPrescaler)
{
    rTCFG0 &= 0xFFFFFF00;
    rTCFG0 |= ucPrescaler;
}
/*********************************************************************************************************
** Function name:           timerSetPrescaler1
** Descriptions:            ���ö�ʱ��2 3 �붨ʱ��4 ��Ԥ��Ƶ����
** input parameters:        ucPrescaler     �µ�Ԥ��Ƶ����
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
void  timerSetPrescaler1 (unsigned char   ucPrescaler)
{
    rTCFG0 &= 0xFFFF00FF;
    rTCFG0 |= (unsigned int)(ucPrescaler << 8);
}
/*********************************************************************************************************
** Function name:           timerSetPrescaler
** Descriptions:            ���ö�ʱ��Ԥ��Ƶ����
** input parameters:        ucGroup         ��ʱ���飬0:��ʱ��0,1   1:��ʱ�� 2,3,4
**                          ucPrescaler     �µ�Ԥ��Ƶ����
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
int  timerSetPrescaler (unsigned char  ucGroup, unsigned char  ucPrescaler)
{
    switch (ucGroup) {
    
    case 0:                                                             /*  ��ʱ���� 0                  */
        timerSetPrescaler0(ucPrescaler);
        break;
        
    case 1:                                                             /*  ��ʱ���� 1                  */
        timerSetPrescaler1(ucPrescaler);
        break;
        
    default:                                                            /*  ��ʱ�������                */
        return  (-1);
    }
    
    return  (0);
}
/*********************************************************************************************************
** Function name:           timerSetDeadZone
** Descriptions:            ���� ��ʱ��0 ������
** input parameters:        ucDeadZone      ������С
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
void  timerSetDeadZone (unsigned char  ucDeadZone)
{
    rTCFG0 &= 0xFF00FFFF;
    rTCFG0 |= (unsigned int)(ucDeadZone << 16);
}
/*********************************************************************************************************
** Function name:           timerSetMuxCnt
** Descriptions:            ���ö�ʱ���ķ�Ƶ��
** input parameters:        ucTimer             ��ʱ��
**                          ucDivider           ��Ƶֵ   0000 = 1/2  0001 = 1/4 0010 = 1/8
**                                                       0011 = 1/16 01xx = External TCLKx
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
int  timerSetMuxCnt (unsigned char  ucTimer, unsigned char  ucDivider)
{
    switch (ucTimer) {
    
    case 0:                                                             /*  ��ʱ�� 0                    */
        rTCFG1 &= 0xFFFFFFF0;
        rTCFG1 |= ucDivider;
        break;
        
    case 1:                                                             /*  ��ʱ�� 1                    */
        rTCFG1 &= 0xFFFFFF0F;
        rTCFG1 |= (ucDivider << 4);
        break;
        
    case 2:                                                             /*  ��ʱ�� 2                    */
        rTCFG1 &= 0xFFFFF0FF;
        rTCFG1 |= (unsigned int)(ucDivider << 8);
        break;
        
    case 3:                                                             /*  ��ʱ�� 3                    */
        rTCFG1 &= 0xFFFF0FFF;
        rTCFG1 |= (unsigned int)(ucDivider << 12);
        break;
        
    case 4:                                                             /*  ��ʱ�� 4                    */
        rTCFG1 &= 0xFFF0FFFF;
        rTCFG1 |= (unsigned int)(ucDivider << 16);
        break;
        
    default:                                                            /*  ��ʱ������                  */
        return  (-1);
    }
    
    return  (0);
}
/*********************************************************************************************************
** Function name:           timerSetDMA
** Descriptions:            ���ö�ʱ���� DMA ����
** input parameters:        ����DMA�Ķ�ʱ����   Ϊ 0 �����ӣ� 1 Ϊ TIMER0 ��������
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
int  timerSetDMA (unsigned char ucTimer)
{
    if (ucTimer > 5) {
        return  (-1);
    }
    
    rTCFG1 &= 0xFF0FFFFF;
    rTCFG1 |= (unsigned int)(ucTimer << 20);
    
    return  (0);
}
/*********************************************************************************************************
** Function name:           timerConfig
** Descriptions:            ���ö�ʱ������ѡ��
** input parameters:        ucTimer             ��ʱ����
**                          ucOption            ѡ��(timer.h)
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
int  timerConfig (unsigned char  ucTimer, unsigned char   ucOption)
{
    switch (ucTimer) {
    
    case 0:                                                             /*  ��ʱ�� 0                    */
        rTCON &= 0xFFFFFF00;
        rTCON |= ucOption;
        break;
        
    case 1:                                                             /*  ��ʱ�� 1                    */
        rTCON &= 0xFFFFF0FF;
        rTCON |= (unsigned int)(ucOption << 8);
        break;
        
    case 2:                                                             /*  ��ʱ�� 2                    */
        rTCON &= 0xFFFF0FFF;
        rTCON |= (unsigned int)(ucOption << 12);
        break;
        
    case 3:                                                             /*  ��ʱ�� 3                    */
        rTCON &= 0xFFF0FFFF;
        rTCON |= (unsigned int)(ucOption << 16);
        break;
        
    case 4:                                                             /*  ��ʱ�� 4                    */
        rTCON &= 0xFF0FFFFF;
        
        if (ucOption &  TIMER_RELOAD) {                                 /*  timer4 û�� INVERTER λ     */
            ucOption &= 0x07;
            ucOption |= TIMER_INVERTER;                                 /*  INVERTER λ ���� Reload     */
        }
        
        rTCON |= (unsigned int)(ucOption << 20);
        break;
    
    default:
        return  (-1);
    }
        
    return  (0);
}
/*********************************************************************************************************
** Function name:           timerSetCnt
** Descriptions:            ���ö�ʱ����ʼ����ֵ
** input parameters:        ucTimer             ��ʱ����
**                          usCnt               ����ֵ
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
int  timerSetCnt (unsigned char  ucTimer, unsigned short   usCnt)
{
    switch (ucTimer) {
    
    case 0:                                                             /*  ��ʱ�� 0                    */
        rTCNTB0 = usCnt;
        break;
    
    case 1:                                                             /*  ��ʱ�� 1                    */
        rTCNTB1 = usCnt;
        break;
        
    case 2:                                                             /*  ��ʱ�� 2                    */
        rTCNTB2 = usCnt;
        break;
        
    case 3:                                                             /*  ��ʱ�� 3                    */
        rTCNTB3 = usCnt;
        break;
        
    case 4:                                                             /*  ��ʱ�� 4                    */
        rTCNTB4 = usCnt;
        break;
        
    default:
        return  (-1);
    }
    
    return   (0);
}
/*********************************************************************************************************
** Function name:           timerSetCmp
** Descriptions:            ���ö�ʱ���Ƚ�ֵ
** input parameters:        ucTimer             ��ʱ����
**                          usCnt               �Ƚ�ֵ
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
int  timerSetCmp (unsigned char  ucTimer, unsigned short   usCnt)
{
    switch (ucTimer) {
    
    case 0:                                                             /*  ��ʱ�� 0                    */
        rTCMPB0 = usCnt;
        break;
    
    case 1:                                                             /*  ��ʱ�� 1                    */
        rTCMPB1 = usCnt;
        break;
        
    case 2:                                                             /*  ��ʱ�� 2                    */
        rTCMPB2 = usCnt;
        break;
        
    case 3:                                                             /*  ��ʱ�� 3                    */
        rTCMPB3 = usCnt;
        break;
        
    default:                                                            /*  û�ж�ʱ�� 4                */
        return  (-1);
    }
    
    return   (0);
}
/*********************************************************************************************************
** Function name:           timerGetCnt
** Descriptions:            ��ö�ʱ������ֵ
** input parameters:        ucTimer             ��ʱ����
** output parameters:       NONE
** Returned value:          ����ֵ  ���󷵻� -1
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
int  timerGetCnt (unsigned char  ucTimer)
{
    switch (ucTimer) {
    
    case 0:                                                             /*  ��ʱ�� 0                    */
        return  (rTCNTO0);
        
    case 1:                                                             /*  ��ʱ�� 1                    */
        return  (rTCNTO1);
        
    case 2:                                                             /*  ��ʱ�� 2                    */
        return  (rTCNTO2);
        
    case 3:                                                             /*  ��ʱ�� 3                    */
        return  (rTCNTO3);
        
    case 4:                                                             /*  ��ʱ�� 4                    */
        return  (rTCNTO4);
        
    default:
        return  (0);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
