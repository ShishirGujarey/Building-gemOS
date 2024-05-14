#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "square") == 0) {
        char *xyz = "./square";
        char *args[argc - 1];
        args[0] = xyz; 
        for (int i = 2; i < argc-2; i++) {
            args[i - 1] = argv[i]; 
        }
        int x = (int)round(sqrt(atoi(argv[argc - 1])));
        char* ptr = (char*)malloc(8*sizeof(char));
        sprintf(ptr, "%d", x);
        args[argc - 2] = ptr;
        args[argc - 1] = NULL; 
        execv(xyz, args);
    } else if (argc > 1 && strcmp(argv[1], "double") == 0) {
        char *xyz = "./double";
        char *args[argc - 1];
        args[0] = xyz; 
        for (int i = 2; i < argc-2; i++) {
            args[i - 1] = argv[i]; 
        }
        int x = (int)round(sqrt(atoi(argv[argc - 1])));
        char* ptr = (char*)malloc(8*sizeof(char));
        sprintf(ptr, "%d", x);
        args[argc - 2] = ptr;
        args[argc - 1] = NULL; 
        execv(xyz, args);
    } else if (argc > 1 && strcmp(argv[1], "sqroot") == 0) {
        char *xyz = "./sqroot";
        char *args[argc - 1];
        args[0] = xyz; 
        for (int i = 2; i < argc-2; i++) {
            args[i - 1] = argv[i]; 
        }
        int x = (int)round(sqrt(atoi(argv[argc - 1])));
        char* ptr = (char*)malloc(8*sizeof(char));
        sprintf(ptr, "%d", x);
        args[argc - 2] = ptr;
        args[argc - 1] = NULL; 
        execv(xyz, args);
    }else {
        printf("%d\n", (int)round(sqrt(atoi(argv[argc - 1]))));
    }
    return 0;
}
