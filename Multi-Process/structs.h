/*FILE NAME: structs.h
  OVERVIEW: Structs header to support the multi-process main.c file*/
typedef struct{
    int row;
    int column;
}Parameters;

typedef struct{
    int buffer1;
    int buffer2;
    int counter;
    int sems;
    int result;
}Descriptors;

typedef struct{
    sem_t writeLock;
    sem_t counterLock;
    sem_t empty;
    sem_t full;
}Semaphores;

typedef struct{
    int totalValid;
    int row;
    int pid;
    char* val;
    int group;
}Result;
