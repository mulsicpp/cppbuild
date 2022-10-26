#include "CppBuilder.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
    CppBuilder builder(argc, argv);

    printf("output path: %s\n", builder.proj_Info.output_Path.c_str());
    printf("output type: %i\n", builder.proj_Info.output_Type);

    int a = 0;

    {
        int b = 3;
        printf("%p\n", &b);
    }
    int c;

    printf("%p\n", &c);
    printf("%i\n", c);

    return 0;
}