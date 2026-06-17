#include "bptree.h"
#include <stdio.h>

int main(int argc, char const *argv[])
{
    BPtree tree;
    init(&tree);
    int op;
    while (1)
    {
        menu();
        scanf("%d", &op);
        switch (op)
        {
            case 1:
            {
                int val;
                printf("请输入要插入的整数：");
                scanf("%d", &val);
                insert(&tree, val);
                printf("插入完成\n\n");
                break;
            }
            case 2:
            {
                int val;
                printf("请输入要查找的整数：");
                scanf("%d", &val);
                int res = search(&tree, val);
                if (res == 1)
                    printf("查找成功：%d 存在于树中\n", val);
                else
                    printf("查找失败：%d 不存在\n", val);
                printf("\n");
                break;
            }
            case 3:
            {
                int l, r;
                printf("请输入区间起点 start：");
                scanf("%d", &l);
                printf("请输入区间终点 end：");
                scanf("%d", &r);
                range_search(&tree, l, r);
                printf("\n");
                break;
            }
            case 4:
                show(&tree);
                printf("\n");
                break;
            case 5:
                destroy(&tree);
                printf("B+树已全部销毁，内存释放完毕\n\n");
                break;
            case 0:
                // 退出前释放内存，防止内存泄漏
                destroy(&tree);
                printf("已释放资源，程序退出\n");
                return 0;
            default:
                printf("输入序号无效，请重新选择！\n\n");
                break;
        }
    }
}