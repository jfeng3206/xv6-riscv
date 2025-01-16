// calc-test.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int res = 0;
    char op = '/';
    int calculated_result = calculate(1, 1, &op, &res);
    printf("res is: %d, calculated result is %d\n", res, calculated_result);
    exit(0);
}