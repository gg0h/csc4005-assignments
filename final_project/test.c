#include <stdlib.h>
#include <stdio.h>

#define min(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})

int main()
{
    int len = 10, chunk = 4, p_len = 2;
    char s[100] = "ABCDEFGHIJ";

    char * c_addr = s + 2;
    printf("%c\n", *c_addr);

    exit(0);

    int searchableLen = len - (p_len -1 );
    
    int div = searchableLen / chunk;
    int mod = searchableLen % chunk;

    printf("div: %d \n", div);
    printf("mod: %d \n", mod);

    int start, end;

    for (int i = 0; i< chunk; i ++) {
        start = (i * div) + (i < mod ? i : mod);
        end = (i + 1) * div + ((i + 1) < mod ? (i + 1) : mod);

        printf("start: %d \n", start);
        printf("end: %d \n", end);

        if (start == end)
        {
             printf("empty chunk: %d \n", i);
             continue;
        }

        end += (p_len - 1);

        printf("start: %d \n", start);
        printf("end: %d \n", end);
        
        for(int j = start; j < end; j++)
            printf("%c\n", *s + j);
    }
}

// 
// start         i * k + min(i, m)
// end              