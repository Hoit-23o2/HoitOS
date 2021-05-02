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
** ��   ��   ��: hoitFsTreeUtil.c
**
** ��   ��   ��: Pan yanqi (������)
**
** �ļ���������: 2021 �� 03 �� 28 ��
**
** ��        ��: JFFS2-Like ��������ݽṹ
*********************************************************************************************************/

#include "hoitFsTreeUtil.h"

#define RB_LEFT_CHILD(pRbn)          (pRbn->pRbnLeft)
#define RB_RIGHT_CHILD(pRbn)         (pRbn->pRbnRight)
#define RB_PARENT(pRbn)              (pRbn->pRbnParent)
#define RB_GRAND(pRbn)               RB_PARENT(RB_PARENT(pRbn))
#define RB_UNCLE_RIGHT(pRbn)         (RB_RIGHT_CHILD(RB_GRAND(pRbn)))
#define RB_UNCLE_LEFT(pRbn)          (RB_LEFT_CHILD(RB_GRAND(pRbn)))

#define RB_IS_LEFT_CHILD(pRbn)       pRbn == RB_LEFT_CHILD(RB_PARENT(pRbn))
#define RB_IS_RIGHT_CHILD(pRbn)      pRbn == RB_RIGHT_CHILD(RB_PARENT(pRbn))
/*********************************************************************************************************
** ��������: __hoitRbMinimum
** ��������: Ѱ��ĳ�������������С���ӽڵ�
** �䡡��  : pRbTree          �����
**           pRbnRoot          �����������             
** �䡡��  : ������С�ӽڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_RB_NODE __hoitRbMinimum(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbnRoot){
    PHOIT_RB_NODE pRbnTraverse;
    pRbnTraverse = pRbnRoot;
    while (pRbnTraverse->pRbnLeft != pRbTree->pRbnGuard)
    {
        pRbnTraverse = RB_LEFT_CHILD(pRbnTraverse);   
    }
    return pRbnTraverse;
}
/*********************************************************************************************************
** ��������: __hoitRbMaximum
** ��������: Ѱ��ĳ����������������ӽڵ�
** �䡡��  : pRbTree          �����
**           pRbnRoot          �����������             
** �䡡��  : ��������ӽڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_RB_NODE __hoitRbMaximum(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbnRoot){
    PHOIT_RB_NODE pRbnTraverse;
    pRbnTraverse = pRbnRoot;
    while (pRbnTraverse->pRbnRight != pRbTree->pRbnGuard)
    {
        pRbnTraverse = RB_RIGHT_CHILD(pRbnTraverse);   
    }
    return pRbnTraverse;
}
/*********************************************************************************************************
** ��������: __hoitRbConquer
** ��������: �������߽ڵ㼰�������滻Ŀ��ڵ�
** �䡡��  : pRbTree          �����
**           pRbnTarget       Ŀ��ڵ�      
**           pRbnConqueror    �����߽ڵ�        
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __hoitRbConquer(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbnTarget, PHOIT_RB_NODE pRbnConqueror){
    if(RB_PARENT(pRbnTarget) == pRbTree->pRbnGuard){
        pRbTree->pRbnRoot = pRbnConqueror;
    }
    else if (RB_IS_LEFT_CHILD(pRbnTarget))
    {
        pRbnTarget->pRbnParent->pRbnLeft = pRbnConqueror;
    }
    else pRbnTarget->pRbnParent->pRbnRight = pRbnConqueror;

    pRbnConqueror->pRbnParent = pRbnTarget->pRbnParent;
}
/*********************************************************************************************************
** ��������: __hoitFsLeftRotate
** ��������: ���������
** �䡡��  : pRbTree          �����
**           pRbn              ������ڵ�
** �䡡��  : �ɹ�����True��ʧ�ܷ���False
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoitRbLeftRotate(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn){
    PHOIT_RB_NODE           pRbnRight;
    
    pRbnRight = pRbn->pRbnRight;
    
    if(pRbnRight == pRbTree->pRbnGuard){
        printf("left rotate: target's right child can't be null\n");
        return FALSE;
    }
    
    pRbn->pRbnRight = RB_LEFT_CHILD(pRbnRight);         /* �ƶ���ǰ�ڵ��Һ��ӵ��������ýڵ���ҽڵ�֮�� */
    
    if(pRbnRight->pRbnLeft != pRbTree->pRbnGuard){      /* ���õ�ǰ�ڵ��Һ��ӵ����ӽڵ�ĸ��׽ڵ㣺 ����ýڵ��ǿգ��������ã���������Ϊ��ǰ�ڵ�*/
        pRbnRight->pRbnLeft->pRbnParent = pRbn;
    }

    pRbnRight->pRbnParent = RB_PARENT(pRbn);            /* ���õ�ǰ�ڵ��Һ��ӵĸ��ڵ� */
    
    if(pRbn->pRbnParent == pRbTree->pRbnGuard){         /* �����ǰ�ڵ�ĸ���Ϊ�գ���ǰ�ڵ���ҽڵ�Ϊ������ĸ� */
        pRbTree->pRbnRoot = pRbnRight;
    }
    else if(pRbn == pRbn->pRbnParent->pRbnLeft){        /* ��ǰ�ڵ������� */
        pRbn->pRbnParent->pRbnLeft = pRbnRight;
    }
    else pRbn->pRbnParent->pRbnRight = pRbnRight;       /* ��ǰ�ڵ����Һ��� */
    
    pRbnRight->pRbnLeft = pRbn;
    pRbn->pRbnParent = pRbnRight;

    return TRUE;
}
/*********************************************************************************************************
** ��������: __hoitFsRightRotate
** ��������: ���������
** �䡡��  : pRbTree          �����
**           pRbn              ������ڵ�
** �䡡��  : �ɹ�����True��ʧ�ܷ���False
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoitRbRightRotate(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn){
    PHOIT_RB_NODE           pRbnLeft;
    
    pRbnLeft = pRbn->pRbnLeft;
    
    if(pRbnLeft == pRbTree->pRbnGuard){
        printf("right rotate: target's left child can't be null\n");
        return FALSE;
    }
    
    pRbn->pRbnLeft = RB_RIGHT_CHILD(pRbnLeft);              
    
    if(pRbnLeft->pRbnRight != pRbTree->pRbnGuard){      
        pRbnLeft->pRbnRight->pRbnParent = pRbn;
    }

    pRbnLeft->pRbnParent = RB_PARENT(pRbn);           
    
    if(pRbn->pRbnParent == pRbTree->pRbnGuard){         
        pRbTree->pRbnRoot = pRbnLeft;
    }
    else if(pRbn == pRbn->pRbnParent->pRbnLeft){        /* ��ǰ�ڵ������� */
        pRbn->pRbnParent->pRbnLeft = pRbnLeft;
    }
    else pRbn->pRbnParent->pRbnRight = pRbnLeft;       /* ��ǰ�ڵ����Һ��� */
    
    pRbnLeft->pRbnRight = pRbn;
    pRbn->pRbnParent = pRbnLeft;

    return TRUE;
}

/*********************************************************************************************************
** ��������: __hoitRbInsertFixUp
** ��������: �ػ�ڵ���ɫ
** �䡡��  : pRbTree          �����
**           pRbn              �����������ڵ�
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __hoitRbInsertFixUp(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn){
    while (RB_PARENT(pRbn)->uiColor == RB_RED)
    {
        if(RB_IS_LEFT_CHILD(RB_PARENT(pRbn))){
            /* ����������, ����ڵ�Ϊ�Ҳ�, ���Ǻ�ɫ 
                       PP(B)
                      /   \
                   P(R)    uncle(R)         
                   /
                pRbn(R)
                        
                        ��

                        PP(B)
                      /   \
                   P(R)    uncle(R)         
                      \
                    pRbn(R)
            */
            if(RB_UNCLE_RIGHT(pRbn)->uiColor == RB_RED){
                RB_PARENT(pRbn)->uiColor = RB_BLACK;
                RB_UNCLE_RIGHT(pRbn)->uiColor = RB_BLACK;
                RB_GRAND(pRbn)->uiColor = RB_RED;
                pRbn = RB_GRAND(pRbn);
            }
            /* �ýڵ����Һ��ӣ������Ǻ�ɫ
                      PP(B)
                      /   \
                   P(R)    uncle(B)
                      \
                    pRbn(R)
            */
            else if(RB_IS_RIGHT_CHILD(pRbn)){
                pRbn = RB_PARENT(pRbn);
                __hoitRbLeftRotate(pRbTree, pRbn);
            }
            /* �ýڵ�������Ǻ�ɫ�ģ����Լ�������
                      PP(B)
                      /   \
                   P(R)    uncle(B)
                   /
                pRbn(R)
            */
            if(RB_IS_LEFT_CHILD(pRbn) && RB_UNCLE_RIGHT(pRbn)->uiColor == RB_BLACK){
                RB_PARENT(pRbn)->uiColor = RB_BLACK;
                RB_GRAND(pRbn)->uiColor = RB_RED;
                __hoitRbRightRotate(pRbTree, RB_GRAND(pRbn));
            }
        }
        else {
            /* �������Һ���, ����ڵ�Ϊ���, ���Ǻ�ɫ */
            if(RB_UNCLE_LEFT(pRbn)->uiColor == RB_RED){
                RB_PARENT(pRbn)->uiColor = RB_BLACK;
                RB_UNCLE_LEFT(pRbn)->uiColor = RB_BLACK;
                RB_GRAND(pRbn)->uiColor = RB_RED;
                pRbn = RB_GRAND(pRbn);
            }
            /* �ýڵ����Һ��� */
            else if(RB_IS_LEFT_CHILD(pRbn)){
                pRbn = RB_PARENT(pRbn);
                __hoitRbRightRotate(pRbTree, pRbn);
            }

            if(RB_IS_RIGHT_CHILD(pRbn) && RB_UNCLE_LEFT(pRbn)->uiColor == RB_BLACK){
                RB_PARENT(pRbn)->uiColor = RB_BLACK;
                RB_GRAND(pRbn)->uiColor = RB_RED;
                __hoitRbLeftRotate(pRbTree, RB_GRAND(pRbn));
            }
        }
    }
    pRbTree->pRbnRoot->uiColor = RB_BLACK;
}

/*********************************************************************************************************
** ��������: __hoitRbDeleteFixUp
** ��������: �ػ�ڵ���ɫ
** �䡡��  : pRbTree          �����
**           pRbn             �����µĺ�����ڵ�
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __hoitRbDeleteFixUp(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn){
    PHOIT_RB_NODE           pRbnBrother;

    while (pRbn != pRbTree->pRbnRoot && pRbn->uiColor == RB_BLACK)              /* �������½ڵ�û�й����µ����ڵ㣬�Ҵ����½ڵ�Ϊ��ɫ���ο��㷨���۵����� P 201 */
    {
        if(RB_IS_LEFT_CHILD(pRbn)){                                             /* ���Ǵ����½ڵ������� */
            pRbnBrother = RB_RIGHT_CHILD(RB_PARENT(pRbn));
            if(pRbnBrother->uiColor == RB_RED){                                 /* ���1�������½ڵ���ֵܽڵ�Ϊ��ɫ������תΪ��ɫ��������2��3��4 */
                pRbnBrother->uiColor = RB_BLACK;
                pRbnBrother->pRbnParent->uiColor = RB_RED;
                __hoitRbLeftRotate(pRbTree, RB_PARENT(pRbn));
                pRbnBrother = RB_RIGHT_CHILD(RB_PARENT(pRbn));
            }
            if(RB_LEFT_CHILD(pRbnBrother)->uiColor == RB_BLACK 
               && RB_RIGHT_CHILD(pRbnBrother)->uiColor == RB_BLACK){            /* ���2�������½ڵ���ֵܽڵ�Ķ��ӽڵ��Ϊ��ɫ */
                pRbnBrother->uiColor = RB_RED;
                pRbn = RB_PARENT(pRbn);
            }
            else if (RB_RIGHT_CHILD(pRbnBrother)->uiColor == RB_BLACK)          /* ���3�������½ڵ���ֵܽڵ���Һ����Ǻ�ɫ */
            {
                pRbnBrother->pRbnLeft->uiColor = RB_BLACK;
                pRbnBrother->uiColor = RB_RED;
                __hoitRbRightRotate(pRbTree, pRbnBrother);
                pRbnBrother = RB_RIGHT_CHILD(RB_PARENT(pRbn));

                pRbnBrother->uiColor = RB_PARENT(pRbn)->uiColor;                /* ���4�������½ڵ���ֵܽڵ���Һ����Ǻ�ɫ */
                pRbn->pRbnParent->uiColor = RB_BLACK;
                RB_RIGHT_CHILD(pRbnBrother)->uiColor = RB_BLACK;
                __hoitRbLeftRotate(pRbTree, RB_PARENT(pRbn));
                pRbn = pRbTree->pRbnRoot;
            }
            else {
                pRbnBrother->uiColor = RB_PARENT(pRbn)->uiColor;                /* ���4�������½ڵ���ֵܽڵ���Һ����Ǻ�ɫ */
                pRbn->pRbnParent->uiColor = RB_BLACK;
                RB_RIGHT_CHILD(pRbnBrother)->uiColor = RB_BLACK;
                __hoitRbLeftRotate(pRbTree, RB_PARENT(pRbn));
                pRbn = pRbTree->pRbnRoot;
            }
        }
        else
        {
            pRbnBrother = RB_LEFT_CHILD(RB_PARENT(pRbn));
            if(pRbnBrother->uiColor == RB_RED){
                pRbnBrother->uiColor = RB_BLACK;
                pRbnBrother->pRbnParent->uiColor = RB_RED;
                __hoitRbRightRotate(pRbTree, RB_PARENT(pRbn));
                pRbnBrother = RB_LEFT_CHILD(RB_PARENT(pRbn));
            }
            if(RB_LEFT_CHILD(pRbnBrother)->uiColor == RB_BLACK 
               && RB_RIGHT_CHILD(pRbnBrother)->uiColor == RB_BLACK){
                pRbnBrother->uiColor = RB_RED;
                pRbn = RB_PARENT(pRbn);
            }
            else if (RB_LEFT_CHILD(pRbnBrother)->uiColor == RB_BLACK)
            {
                pRbnBrother->pRbnRight->uiColor = RB_BLACK;
                pRbnBrother->uiColor = RB_RED;
                __hoitRbLeftRotate(pRbTree, pRbnBrother);
                pRbnBrother = RB_LEFT_CHILD(RB_PARENT(pRbn));

                pRbnBrother->uiColor = RB_PARENT(pRbn)->uiColor;
                pRbn->pRbnParent->uiColor = RB_BLACK;
                RB_LEFT_CHILD(pRbnBrother)->uiColor = RB_BLACK;
                __hoitRbRightRotate(pRbTree, RB_PARENT(pRbn));
                pRbn = pRbTree->pRbnRoot;
            }
            else {
                pRbnBrother->uiColor = RB_PARENT(pRbn)->uiColor;
                pRbn->pRbnParent->uiColor = RB_BLACK;
                RB_LEFT_CHILD(pRbnBrother)->uiColor = RB_BLACK;
                __hoitRbRightRotate(pRbTree, RB_PARENT(pRbn));
                pRbn = pRbTree->pRbnRoot;
            }
        }
        
    }
    pRbn->uiColor = RB_BLACK;
}
/*********************************************************************************************************
** ��������: __hoitRbTraverse
** ��������: ������������
** �䡡��  : pRbTree          �����
**           pRbnRoot          ����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __hoitRbTraverse(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbnRoot){
    if(pRbnRoot == pRbTree->pRbnGuard){
        return;
    }
    else
    {
        __hoitRbTraverse(pRbTree, pRbnRoot->pRbnLeft);
        printf("%d: %s left: %s right: %s\n", pRbnRoot->iKey, 
                                              pRbnRoot->uiColor == RB_RED ? "RED" : "BLACK", 
                                              RB_LEFT_CHILD(pRbnRoot)->uiColor == RB_RED ? "RED" : "BLACK", 
                                              RB_RIGHT_CHILD(pRbnRoot)->uiColor == RB_RED ? "RED" : "BLACK");
        __hoitRbTraverse(pRbTree, pRbnRoot->pRbnRight);   
    }
}


/*********************************************************************************************************
** ��������: hoitRbInsertNode
** ��������: ����һ��������ڵ�
** �䡡��  : pRbTree          �����
**           pRbn               ������ڵ�              
** �䡡��  : ���ز���Ľڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_RB_NODE hoitRbInsertNode(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn){
    PHOIT_RB_NODE       pRbnTrailing;
    PHOIT_RB_NODE       pRbnTraverse;

    pRbnTrailing = pRbTree->pRbnGuard;
    pRbnTraverse = pRbTree->pRbnRoot;
    
    while (pRbnTraverse != pRbTree->pRbnGuard)
    {
        pRbnTrailing = pRbnTraverse;
        if(pRbn->iKey < pRbnTraverse->iKey){
            pRbnTraverse = pRbnTraverse->pRbnLeft;
        }
        else pRbnTraverse = pRbnTraverse->pRbnRight;
    }

    pRbn->pRbnParent = pRbnTrailing;

    if(pRbnTrailing == pRbTree->pRbnGuard){
        pRbTree->pRbnRoot = pRbn;
    }
    else if(pRbn->iKey < pRbnTrailing->iKey){
        pRbnTrailing->pRbnLeft = pRbn;
    }
    else pRbnTrailing->pRbnRight = pRbn;

    pRbn->pRbnLeft = pRbTree->pRbnGuard;
    pRbn->pRbnRight = pRbTree->pRbnGuard;
    pRbn->uiColor = RB_RED;
    __hoitRbInsertFixUp(pRbTree, pRbn);

    return pRbn;
}


/*********************************************************************************************************
** ��������: hoitRbSearchNode
** ��������: ���ݼ�ֵ���Һ�����ڵ�
** �䡡��  : pRbTree          �����
**           iKey              ��ֵ              
** �䡡��  : �ɹ����ؽڵ�ָ�룬���򷵻�LW_NULL
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_RB_NODE hoitRbSearchNode(PHOIT_RB_TREE pRbTree, INT32 iKey){
    PHOIT_RB_NODE       pRbnTraverse;

    pRbnTraverse = pRbTree->pRbnRoot;

    while (pRbnTraverse != pRbTree->pRbnGuard)
    {
        if(pRbnTraverse->iKey == iKey){
            return pRbnTraverse;
        }
        else if(pRbnTraverse->iKey <= iKey){
            pRbnTraverse = RB_RIGHT_CHILD(pRbnTraverse);
        }
        else{
            pRbnTraverse = RB_LEFT_CHILD(pRbnTraverse);
        }
    }
    return pRbTree->pRbnGuard;
}

/*********************************************************************************************************
** ��������: hoitRbDeleteNode
** ��������: ɾ��һ��������ڵ㣬ע�⣬���ǲ������ͷ��ڴ棬����Ϊ�˺�������
** �䡡��  : pRbTree          �����
**           pRbn             ��ɾ���ڵ�              
** �䡡��  : �ɹ�����True��ʧ�ܷ���False
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL hoitRbDeleteNode(PHOIT_RB_TREE pRbTree, PHOIT_RB_NODE pRbn){
    PHOIT_RB_NODE           pRbnTraverse;
    PHOIT_RB_NODE           pRbnConqueror;
    UINT32                  uiTraverseOriginColor;

    pRbnTraverse = pRbn;                            /* ��¼��ɾ���ڵ���ɫ */
    uiTraverseOriginColor = pRbnTraverse->uiColor;
    /* ��ɾ���ڵ���Һ���Ϊ�գ���ֱ���ƶ� (ͬ������LC��NIL�����)
              |                             |
            pRbn            =>             LC
            /  \                          /  \
           LC   NIL
    */
    if(RB_RIGHT_CHILD(pRbn) == pRbTree->pRbnGuard){
        pRbnConqueror = RB_LEFT_CHILD(pRbn);
        __hoitRbConquer(pRbTree, pRbn, pRbnConqueror);
    }
    /* ��ɾ���ڵ������Ϊ�գ���ֱ���ƶ� (ͬ������RC��NIL�����)
              |                             |
            pRbn            =>             RC
            /  \                          /  \
           NIL RC  
    */
    else if (RB_LEFT_CHILD(pRbn) == pRbTree->pRbnGuard)
    {
        pRbnConqueror = RB_RIGHT_CHILD(pRbn);
        __hoitRbConquer(pRbTree, pRbn, pRbnConqueror);
    }
    else
    {
        pRbnTraverse = __hoitRbMinimum(pRbTree, RB_RIGHT_CHILD(pRbn));        /* Ѱ�Һ��Ԫ�� */
        uiTraverseOriginColor = pRbnTraverse->uiColor;                        /* ɾ��pRbn�ڵ㣬�൱�ڰ�pRbn��ֵ���̽ڵ㽻����Ȼ��ɾ����̽ڵ㣬�����������ɾ���ڵ����ɫ*/
        pRbnConqueror = RB_RIGHT_CHILD(pRbnTraverse);                             
        
        if(RB_PARENT(pRbnTraverse) == pRbn){                                  /* https://www.zhihu.com/question/38296405 */
            pRbnConqueror->pRbnParent = pRbnTraverse;                           /* pRbnConqueror����ΪpRbTree.Guard */
        }
        else
        {
            /* ��ɾ���ڵ������Ϊ�գ���ֱ���ƶ� (ͬ������LC��NIL�����)
                      |                             |                               |
                    pRbn            =>             pRbn  S          =>              S
                    /  \                          /  \  /                         /  \
                   LC  RC                        LC  RC                          LC  RC
                     ... \                         ... \                           ... \
                     /                             /                                /
                    S                             SRC                             SRC
                  /  \                           /  \                            /  \
                NIL  SRC
            */
            __hoitRbConquer(pRbTree, pRbnTraverse, pRbnConqueror);
            pRbnTraverse->pRbnRight = RB_RIGHT_CHILD(pRbn);
            pRbnTraverse->pRbnRight->pRbnParent = pRbnTraverse;
        }
        
        __hoitRbConquer(pRbTree, pRbn, pRbnTraverse);
        pRbnTraverse->pRbnLeft = pRbn->pRbnLeft;
        pRbnTraverse->pRbnLeft->pRbnParent = pRbnTraverse;
        pRbnTraverse->uiColor = pRbn->uiColor;                              /* ��Ϊ���������԰�S����ɫ��һ�� */
    }
    
    if(uiTraverseOriginColor == RB_BLACK){          /* ɾ����ڵ㲻��Ӱ������ƽ�⣬ɾ���ڽڵ�ҪӰ�죬������Ҫ���� */
        __hoitRbDeleteFixUp(pRbTree, pRbnConqueror);
    }

    return LW_TRUE;
}

/*********************************************************************************************************
** ��������: hoitInitRbTree
** ��������: �������ʼ��
** �䡡��  : NONE
** �䡡��  : �ɹ�����True��ʧ�ܷ���False
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_RB_TREE hoitRbInitTree(){
    PHOIT_RB_TREE           pRbTree; 

    pRbTree = (PHOIT_RB_TREE)lib_malloc(sizeof(HOIT_RB_TREE));
    pRbTree->pRbnGuard = (PHOIT_RB_NODE)lib_malloc(sizeof(HOIT_RB_NODE));
    if(pRbTree < 0 || pRbTree->pRbnGuard < 0){
        printf("init red/black tree fail: memory low\n");
        return LW_NULL;
    }
    pRbTree->pRbnGuard->uiColor = RB_BLACK;
    pRbTree->pRbnRoot = pRbTree->pRbnGuard;
    return pRbTree;
}

#ifdef RB_TEST
VOID hoitRbTreeTest(){
    INT i;
    PHOIT_RB_TREE pRbTree;
    PHOIT_RB_NODE pRbn;
    INT testArray[10] = {8,11,14,15,1,2,4,5,7, 1};

    printf("\n [Red Black Tree Test Start] \n");
    pRbTree = hoitRbInitTree();
    
    for (i = 0; i < 10; i++)
    {
        pRbn = newHoitRbNode(testArray[i]);
        hoitRbInsertNode(pRbTree, pRbn);
    }
    pRbn = hoitRbSearchNode(pRbTree, 7);
    
    printf("[test traverse] \n");
    __hoitRbTraverse(pRbTree, pRbTree->pRbnRoot);
    printf("[test delete 7] \n");
    
    hoitRbDeleteNode(pRbTree, pRbn);
    lib_free(pRbn);
    
    __hoitRbTraverse(pRbTree, pRbTree->pRbnRoot);
    
}
#endif // DEBUG
