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
** ��   ��   ��: timer.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 12 �� 09 ��
**
** ��        ��: S3C2440 Ӳ����ʱ��.
*********************************************************************************************************/

#ifndef __TIMER_H
#define __TIMER_H

/*********************************************************************************************************
  ��ʱ�����ò���
*********************************************************************************************************/

#define TIMER_ENABLE            (1 << 0)                                /*  ������ʱ��                  */
#define TIMER_DISABLE           (0 << 0)                                /*  ֹͣ��ʱ��                  */

#define TIMER_MANUAL_UPDATE     (1 << 1)                                /*  ��ʱ���ֶ�����              */
#define TIMER_UNMANUAL_UPDATE   (0 << 1)                                /*  ��ʱ���Զ�����              */

#define TIMER_INVERTER          (1 << 2)                                /*  ��ʱ�������ת              */
#define TIMER_UNINVERTER        (0 << 2)                                /*  ��ʱ���������ת            */

#define TIMER_RELOAD            (1 << 3)                                /*  ѭ������                    */
#define TIMER_UNRELOAD          (0 << 3)                                /*  ���μ���                    */

#define TIMER_DEADZONE          (1 << 4)                                /*  ��ʱ��0ʹ������             */
#define TIMER_UNDEAD_ZONE       (0 << 4)                                /*  ��ʱ��0��ʹ������           */

/*********************************************************************************************************
  ��ʱ����������
*********************************************************************************************************/
/*********************************************************************************************************
** Function name:           timerSetPrescaler0
** Descriptions:            ���ö�ʱ��0 �� ��ʱ��1 ��Ԥ��Ƶ����
** input parameters:        ucPrescaler     �µ�Ԥ��Ƶ����
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
void    timerSetPrescaler0(unsigned char   ucPrescaler);
/*********************************************************************************************************
** Function name:           timerSetPrescaler1
** Descriptions:            ���ö�ʱ��2 3 �붨ʱ��4 ��Ԥ��Ƶ����
** input parameters:        ucPrescaler     �µ�Ԥ��Ƶ����
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
void    timerSetPrescaler1(unsigned char   ucPrescaler);
/*********************************************************************************************************
** Function name:           timerSetPrescaler
** Descriptions:            ���ö�ʱ��Ԥ��Ƶ����
** input parameters:        ucGroup         ��ʱ���飬0:��ʱ��0,1   1:��ʱ�� 2,3,4
**                          ucPrescaler     �µ�Ԥ��Ƶ����
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
*********************************************************************************************************/
int     timerSetPrescaler(unsigned char  ucGroup, unsigned char  ucPrescaler);
/*********************************************************************************************************
** Function name:           timerSetDeadZone
** Descriptions:            ���� ��ʱ��0 ������
** input parameters:        ucDeadZone      ������С
** output parameters:       NONE
** Returned value:          NONE
*********************************************************************************************************/
void    timerSetDeadZone(unsigned char  ucDeadZone);
/*********************************************************************************************************
** Function name:           timerSetMuxCnt
** Descriptions:            ���ö�ʱ���ķ�Ƶ��
** input parameters:        ucTimer             ��ʱ��
**                          ucDivider           ��Ƶֵ   0000 = 1/2  0001 = 1/4 0010 = 1/8
**                                                       0011 = 1/16 01xx = External TCLKx
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
*********************************************************************************************************/
int     timerSetMuxCnt(unsigned char  ucTimer, unsigned char  ucDivider);
/*********************************************************************************************************
** Function name:           timerSetDMA
** Descriptions:            ���ö�ʱ���� DMA ����
** input parameters:        ����DMA�Ķ�ʱ����   Ϊ 0 �����ӣ� 1 Ϊ TIMER0 ��������
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
*********************************************************************************************************/
int     timerSetDMA(unsigned char ucTimer);
/*********************************************************************************************************
** Function name:           timerConfig
** Descriptions:            ���ö�ʱ������ѡ��
** input parameters:        ucTimer             ��ʱ����
**                          ucOption            ѡ��(timer.h)
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
*********************************************************************************************************/
int     timerConfig(unsigned char  ucTimer, unsigned char   ucOption);
/*********************************************************************************************************
** Function name:           timerSetCnt
** Descriptions:            ���ö�ʱ����ʼ����ֵ
** input parameters:        ucTimer             ��ʱ����
**                          usCnt               ����ֵ
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
*********************************************************************************************************/
int     timerSetCnt(unsigned char  ucTimer, unsigned short   usCnt);
/*********************************************************************************************************
** Function name:           timerSetCmp
** Descriptions:            ���ö�ʱ���Ƚ�ֵ
** input parameters:        ucTimer             ��ʱ����
**                          usCnt               �Ƚ�ֵ
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
*********************************************************************************************************/
int     timerSetCmp(unsigned char  ucTimer, unsigned short   usCnt);
/*********************************************************************************************************
** Function name:           timerGetCnt
** Descriptions:            ��ö�ʱ������ֵ
** input parameters:        ucTimer             ��ʱ����
** output parameters:       NONE
** Returned value:          ����ֵ  ���󷵻� -1
*********************************************************************************************************/
int     timerGetCnt(unsigned char  ucTimer);

#endif                                                                  /*  __TIMER_H                   */
/*********************************************************************************************************
  END
*********************************************************************************************************/

