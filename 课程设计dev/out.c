int g_dec = 123;

int g_oct = 0123;

int g_hex = 0x123;

long g_long = 123L;

float g_pi = 3.14;

char g_ch = 'A';

int g_arr[10];

int max(int a, int b);

void print_msg(void);

int max(int a, int b)
{
    int result;
    if (a > b)
        result = a;
    else
        result = b;
    return result;
}

int loop_test(int n)
{
    int i, j, total = 0;
    while (i <= n)
    {
        if (i == 3)
        {
            i = i + 1;
            continue;
        }
        if (i > 5)
            break;
        total = total + i;
        i = i + 1;
    }
    for (i = 0; i < 3; i = i + 1)
    {
        for (j = 0; j < 2; j = j + 1)
            total = total + 1;
    }
    return total;
}

int nested_if(int a, int b)
{
    int r;
    if ((a >= 0) && (b >= 0))
    {
        if (a == b)
            r = 1;
        else
            if (a != b)
            {
                if (a < b)
                    r = 2;
                else
                    r = 3;
            }
    }
    else
        if ((a <= 0) || (b <= 0))
            r = 4;
        else
        {
            r = 5;
            return r;
        }
}

void print_msg(void)
{
    g_arr[0] = 10;
    g_arr[1] = 20;
    printf("Hello, result = %d\n", g_arr[0] + g_arr[1]);
}

int main(void)
{
    int x = 10, y = 20, z;
    float avg;
    char c = 'B';
    z = x + y;
    z = z - 5;
    z = z * 2;
    z = z / 3;
    z = z % 7;
    z = max(x, y) + (loop_test(5) % 3);
    z = nested_if(max(x, y), y);
    avg = (x + y) / 2.0;
    print_msg();
    printf("z=%d, avg=%f, c=%c\n", z, avg, c);
    return 0;
}

