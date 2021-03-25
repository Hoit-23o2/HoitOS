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
** ��   ��   ��: targetInit.h
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 12 �� 09 ��
**
** ��        ��: S3C2440 �ؼ�Ӳ����ʼ��.
*********************************************************************************************************/

#ifndef __TARGETINIT_H
#define __TARGETINIT_H

/*********************************************************************************************************
  ϵͳ��������
*********************************************************************************************************/
extern  const unsigned int  __GuiPllFreqTbl[2][7];
/*********************************************************************************************************
  ϵͳ��Ƶ����
*********************************************************************************************************/

#define FCLK              (__GuiPllFreqTbl[INPUT_FREQ][MPLL_FREQ])      /*  ϵͳ��Ƶ                    */
#define HCLK              (FCLK / AHB_DIV)                              /*  ϵͳ��Ƶ                    */
#define PCLK              (HCLK / APB_DIV)                              /*  ϵͳƬ������Ƶ��            */
#define UCLK              (48000000)                                    /*  UCLK must be 48MHz for USB  */

/*********************************************************************************************************
  ARM ģʽ����
*********************************************************************************************************/

#define MODE_USR32         0x10                                         /*  �û�״̬                    */
#define MODE_FIQ32         0x11                                         /*  �����ж�״̬                */
#define MODE_IRQ32         0x12                                         /*  �ж�״̬                    */
#define MODE_SVC32         0x13                                         /*  ����״̬                    */
#define MODE_ABT32         0x17                                         /*  ��ֹ״̬                    */
#define MODE_UND32         0x1B                                         /*  δ����״̬                  */
#define MODE_SYS32         0x1F                                         /*  ϵͳ״̬                    */

#define MODE_DISFIQ        0x40                                         /*  �ر� FIQ �ж�               */
#define MODE_DISIRQ        0x80                                         /*  �ر� IRQ �ж�               */

/*********************************************************************************************************
  �ж���������
*********************************************************************************************************/

#define VIC_CHANNEL_EINT0           0                                   /*  �ⲿ�ж� 0                  */
#define VIC_CHANNEL_EINT1           1                                   /*  �ⲿ�ж� 1                  */
#define VIC_CHANNEL_EINT2           2                                   /*  �ⲿ�ж� 2                  */
#define VIC_CHANNEL_EINT3           3                                   /*  �ⲿ�ж� 3                  */
#define VIC_CHANNEL_EINT4_7         4                                   /*  �ⲿ�ж� 4 �� 7             */
#define VIC_CHANNEL_EINT8_23        5                                   /*  �ⲿ�ж� 8 �� 23            */
#define VIC_CHANNEL_CAM             6                                   /*  ����ͷ�ж�                  */
#define VIC_CHANNEL_BAT_FLT         7                                   /*  ��ع���                    */
#define VIC_CHANNEL_TICK            8                                   /*  RTC Tick �ж�               */
#define VIC_CHANNEL_WDT_AC97        9                                   /*  ���Ź��� AC97 �����ӿ��ж�  */
#define VIC_CHANNEL_TIMER0         10                                   /*  ��ʱ�� 0                    */
#define VIC_CHANNEL_TIMER1         11                                   /*  ��ʱ�� 1                    */
#define VIC_CHANNEL_TIMER2         12                                   /*  ��ʱ�� 2                    */
#define VIC_CHANNEL_TIMER3         13                                   /*  ��ʱ�� 3                    */
#define VIC_CHANNEL_TIMER4         14                                   /*  ��ʱ�� 4                    */
#define VIC_CHANNEL_UART2          15                                   /*  ���� 2                      */
#define VIC_CHANNEL_LCD            16                                   /*  LCD ������                  */
#define VIC_CHANNEL_DMA0           17                                   /*  DMA ͨ�� 0                  */
#define VIC_CHANNEL_DMA1           18                                   /*  DMA ͨ�� 1                  */
#define VIC_CHANNEL_DMA2           19                                   /*  DMA ͨ�� 2                  */
#define VIC_CHANNEL_DMA3           20                                   /*  DMA ͨ�� 3                  */
#define VIC_CHANNEL_SDI            21                                   /*  SDI �ж�                    */
#define VIC_CHANNEL_SPI0           22                                   /*  SPI0 �ж�                   */
#define VIC_CHANNEL_UART1          23                                   /*  ���� 1                      */
#define VIC_CHANNEL_NFCON          24                                   /*  NAND FLASH �������ж�       */
#define VIC_CHANNEL_USBD           25                                   /*  USB �豸�ж�                */
#define VIC_CHANNEL_USBH           26                                   /*  USB �������ж�              */
#define VIC_CHANNEL_IIC            27                                   /*  IIC �ж�                    */
#define VIC_CHANNEL_UART0          28                                   /*  ���� 0                      */
#define VIC_CHANNEL_SPI1           29                                   /*  SPI1 �ж�                   */
#define VIC_CHANNEL_RTC            30                                   /*  RTC �����ж�                */
#define VIC_CHANNEL_ADC            31                                   /*  ADC �ж�                    */

/*********************************************************************************************************
  ��ʼ������
*********************************************************************************************************/
void  sdramInit(void);
void  targetInit(void);

#endif                                                                  /*  __TARGETINIT_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
