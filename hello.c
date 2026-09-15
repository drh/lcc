#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *message = malloc(32);

    if (message == NULL)
        return 1;

    strcpy(message, "Hello, LCC 3.6!");

    printf("%s (length=%zu)\n", message, strlen(message));

    free(message);

    return 0;
}
