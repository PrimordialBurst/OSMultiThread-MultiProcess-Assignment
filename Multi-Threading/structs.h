/*FILE NAME: structs.h
  OVERVIEW: Structs header to support the multi-thread main.c file*/

typedef struct{
    int row;
    int column;
}Parameters;

typedef struct{
    int totalValid;
    int row;
    pthread_t tid;
    char* val;
    int group;
}Result;
