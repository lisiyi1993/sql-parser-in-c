#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

void parse_string(char *p, char *value, char *field) {
    char *c;
    char t[256];

    int i=0;
    for (c=p; *c!='\0'; c++) {
        if (*c == '_') {
            t[i] = '\0';
            strcpy(field, t);
            t[0] = 0;
            i=0;
        }
        else {
            t[i] = *c;
            i++;
        }
    }

    t[i] = '\0';
    strcpy(value, t);
}

int main() {
    // srand ( time(NULL) );
    // printf("hello %ld \n", rand());
    // char *tmp = malloc(256);
    // sprintf(tmp, "%d ", 1234);

    // char *str = malloc(1024);;
    // strcat(str, "strings ");
    // strcat(str, "are ");
    // strcat(str, tmp);
    // strcat(str, "concatenated.");
    // printf("hello %s \n", str);

    // char *ptr = "'abcd'";
    // char *str = (char *)malloc(strlen(ptr));

    // strcpy(str, ptr);
    // printf("str = %s \n", str);
    // str = str + 1;
    // str[strlen(str)-1] = '\0';

    // printf("str = %s \n", str);

    // bool b = "123" > "222";
    // printf("%s", b ? "true" : "false");

    char s[] = "123";
    char *p = "abc";
    char *value = calloc(1, sizeof(char *));
    char *field = calloc(1, sizeof(char *));

    parse_string(p, value, field);

    if (strcmp(field, "") == 0) {
        printf("field is null\n");
    }
    else {
        printf("field %s \n", field);
    }
    printf("value %s \n", value);

    return 0;
}