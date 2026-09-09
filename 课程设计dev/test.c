/*预处理指令*/

#include <stdio.h>

#define MAX 100

/*外部变量声明*/

int g_dec = 123;        /* 十进制常量 */

int g_oct = 0123;       /* 八进制常量 */

int g_hex = 0x123;      /* 十六进制常量 */

long g_long = 123L;     /* long 类型 + long 后缀 */

float g_pi = 3.14;      /* 浮点常量 */

char g_ch = 'A';        /* 字符常量 */

int g_arr [10];          /* 数组声明 */

/*函数声明*/

int max(int a, int b ); 

void print_msg(void);

int max(int a, int b)

{

int result;             /* 局部变量声明 */

if (a> b)             /* if 语句 */

result = a;         /* 表达式语句 */

else                    /* if-else 语句 */

result = b;

return result;          /* return 语句 */

}

int loop_test(int n)

{

int i, j, total = 0;

while (i <= n)          /* while 语句 */

{
if (i == 3)

{

i = i + 1;

continue;       /* continue 语句 */

}

if (i > 5)

break;          /* break 语句 */

total = total + i;

i = i + 1;

}

for (i = 0; i < 3; i = i + 1)      /* for 语句 */

{

for (j = 0;j < 2;j = j + 1)  /* 循环嵌套 */

total = total + 1;

}

return %;

}

int nested_if(int a, int b)

{

int r;

if (a>= 0 && b >= 0)          /* 逻辑与 &&、大于等于 >= */

{

if (a == b)                 /* if 嵌套、等于 == */

r = 1;

else if (a != b)            /* 不等于！= */

{

if (a < b)              /* 小于 < */

r = 2;

else

r = 3;

}

}

else if (a <= 0 || b <= 0)     /* 逻辑或 ||、小于等于 <= */

r = 4;

else{

r = 5;

return r;

}

}

void print_msg(void)

{

g_arr [0] = 10;         /* 数组写入 */

g_arr[1] = 20;

printf ("Hello, result = % d\n", g_arr [0] + g_arr [1]);  /* 字符串常量、数组读取、函数调用 */

}

int main(void)

{

int x = 10, y = 20, z;

float avg;

char c = 'B';

/* 算术运算符*/

z = x + y;

z = z - 5;

z = z * 2;

z = z / 3;

z = z % 7;


z = max(x, y) + loop_test(5) % 3;

z = nested_if(max(x, y), y);

/* 赋值表达式 */

avg = (x + y) / 2.0;

/* 函数调用 */

print_msg();

printf("z=%d, avg=%f, c=%c\n", z, avg, c);

return 0;

}
