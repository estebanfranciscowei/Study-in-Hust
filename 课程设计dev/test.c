/* 错误测试用例：包含多种语法错误，用于验证报错功能 */
#include <stdio.h>

int g_x = 10; 
int g_y = 20;

int max(int a, int b)
{
    int result;
    if (a > b
        result = a;
    else
        result = b;
    return result;
}

int main(void)
{
    int x = 1;
    int y = 20;
    int z;

    z = max(x, y);

    printf("result = %d\n", z);
    return 0;
}

