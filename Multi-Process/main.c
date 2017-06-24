/*Name: Jeremy Ciccarelli
  Student ID: 18760376
  Program: Multi-process Sudoku Solution Validator
  Overview: This program reads in a provided sudoku solution in a textfile
            and determines whether or not it is valid, by creating 11 processes
            to validate the 27 separate subgrids of the solution (9 rows,
            9 columns, 9 3x3 subgrids). If all 27 subgrids are valid, then the
            solution will be deemed valid. If any errors are detected within
            the solution, they will be written to the logfile which is created
            at runtime.*/

/*IMPORTS*/
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/shm.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>
#include "structs.h"


//DECLARATIONS
const int bufferSize1 = (sizeof(int) * 81);
const int bufferSize2 = (sizeof(int) * 11);
const int counterSize = (sizeof(int));
const int semsSize = (sizeof(Semaphores));
const int resultSize = (sizeof(Result));
const char* logfile = "logfile.txt";

int (*ptrBuffer1)[9];
int* ptrBuffer2;
int* ptrCounter;
Semaphores* sems;
Result* result;
FILE* writeFile;
Parameters rows[9];
int maxDelay;

/*FUNCTION: ReadFile
  OVERVIEW: This function performs the reading of the specified .txt file specified
            as a command line parameter, assigning it to the 2D array 'grid',
            ptrBuffer1, which is in shared memory.*/

int readFile(char* filename){
    /*Initialise the file pointer 'readFile'*/
    int i = 0;
    int j = 0;
    int k = 0;
    int retVal =0;
    FILE* readFile;
    
    readFile = fopen(filename,"r");
    if(readFile == NULL){
        printf("Error reading file");
    }
    else{
        while(!feof(readFile) && i<9){
            fscanf(readFile, "%1d ",&ptrBuffer1[i][j]);
            k++;
            j++;
            if(j>8){
                i++;
                j=0;
            }
        }
        retVal = 1;
    }
    fclose(readFile);
    if(k != 81){
        retVal = 1;
    }
    return retVal;
}

/*FUNCTION: InitialiseSharedMem
  OVERVIEW: Initialises the values of all elements in ptrBuffer2 to be 0 and sets
            the initial value of the counter (number of valid subgrids) to be 0.*/

void initialiseSharedMem(){
    int i;
    for(i=0;i<11;i++){
        ptrBuffer2[i] = 0;
    }
    *ptrCounter = 0;
}

/*FUNCTION: CheckRow
  OVERVIEW: This function takes in an assigned row of a sudoku grid, checks to make
            sure it's a valid row (contains uniquely numbers 1-9) and passes
            the result (as a theoretical 'producer') to the consumer function
            upon requiring the counter lock. If a row is deemed invalid, it will
            be written to the logfile once the writeLock is acquired.*/

void checkRow(Parameters rc){
    int i,num,valid=0;
    int row = rc.row;
    int usedArray[9] = {0};
    for(i=0;i<9;i++){
        num = ptrBuffer1[row][i];
        if((num < 1)|| (num >9) || usedArray[num-1] !=0){
            valid =1;
        }
        else{
           usedArray[num-1] = 1;
        }
    }
    
    if(valid == 1){
        sem_wait(&sems->writeLock);
                writeFile = fopen(logfile,"a");
                fprintf(writeFile,"process ID-%d: row %d is invalid\n",getpid(),row+1);
                fclose(writeFile);
        sem_post(&sems->writeLock);
    }

    sleep(rand() % maxDelay);    

    ptrBuffer2[row] = 1-valid;
    sem_wait(&sems->empty);
        sem_wait(&sems->counterLock);
            result->row = row;
            result->pid = getpid();
            result->group = 1;
            if(valid == 0){
               result->val = "valid";
               *ptrCounter = *ptrCounter+1;
            }
            else{
                result->val = "invalid";
            }
        sem_post(&sems->counterLock);
    sem_post(&sems->full);    
    
    _exit(0);
 }

/*FUNCTION: CheckColumns
  OVERVIEW: This function checks over each column in the sudoku grid, validating
            each one and maintaining a count (totalValid) which is then passed to
            the consumer process via the result struct. Additionally, any errors
            detected are written to the logfile once the writeLock is acquired.*/

void checkColumns(){
    int i,j,num,curValid,totalValid=0;
    for(i=0;i<9;i++){
        curValid = 0;
        int usedArray[9] = {0};
        for(j=0;j<9;j++){
            num = ptrBuffer1[j][i];
            if((num < 1)|| (num >9) || usedArray[num-1] !=0){
                curValid = 1;
            }
            else{
                usedArray[num-1] = 1;
            }
        }
        if(curValid == 0){
            totalValid++;
        }
        else{
            sem_wait(&sems->writeLock);
                    writeFile = fopen(logfile,"a");
                    fprintf(writeFile,"process ID-%d: column %d is invalid\n",getpid(),i+1);
                    fclose(writeFile);
            sem_post(&sems->writeLock);
        }
    }

    sleep(rand() % maxDelay);   
    
    ptrBuffer2[9] = totalValid;
    sem_wait(&sems->empty);
        sem_wait(&sems->counterLock);
            *ptrCounter = *ptrCounter+totalValid;
            result->pid = getpid();
            result->group = 2;
            result->totalValid = totalValid;
        sem_post(&sems->counterLock);
    sem_post(&sems->full);    
    _exit(0);
}

/*FUNCTION: CheckSubGrids
  OVERVIEW: This function iterates through the 9 3x3 subgrids of the sudoku grid,
            validating each square. Maintains a count of valid subgrids and appends
            error messages detected onto a string which is written to the logfile
            once the writeLock is acquired and results are passed to the consumer process.*/

void checkSubGrids(){
    int i,j,k,l,num,curValid,totalValid=0;
    for(i=0;i<9;i= i+3){
        for(j=0;j<9;j=j+3){
            //SUB GRID START
            curValid = 0;
            int usedArray[9] = {0};
            for(k=i;k<i+3;k++){
                for(l=j;l<j+3;l++){
                    
                    num = ptrBuffer1[k][l];
                    if((num < 1)|| (num >9) || usedArray[num-1] !=0){
                        curValid = 1;
                    }
                    else{
                        usedArray[num-1] = 1;
                    }
                }
            }
            if(curValid == 0){
                totalValid++;
            }
            else{
                sem_wait(&sems->writeLock);
                            writeFile = fopen(logfile,"a");
                            fprintf(writeFile,"process ID-%d: subgrid [%d..%d, %d..%d] is invalid\n",getpid(),i+1,i+3,j+1,j+3);
                            fclose(writeFile);
                sem_post(&sems->writeLock);
            }
        }
    }

    sleep(rand() % maxDelay);   
    
    ptrBuffer2[10] = totalValid;
    sem_wait(&sems->empty);
        sem_wait(&sems->counterLock);
            *ptrCounter = *ptrCounter+totalValid;
            result->pid = getpid();
            result->group = 3;
            result->totalValid = totalValid;
        sem_post(&sems->counterLock);
    sem_post(&sems->full);    
    _exit(0);
}

/*FUNCTION: Consumer
  OVERVIEW: This function performs 11 iterations (corresponding to the 11 calculation
            threads) of waiting for a result to be produced by one of these such threads.
            It prints out the results of each validation, taken from the result struct,
            and at the end of the 11th thread, it checks to see if the counter for
            the number of valid subgrids. If counter is equal to 27, it will indicate
            that the sudoku grid is valid, otherwise deem it invalid. */

void consumer(){
    int i;
    for(i =0;i<11;i++){
        sem_wait(&sems->full);
            sem_wait(&sems->counterLock);
                if(result->group == 1){
                    printf("Validation result from process ID-%d: row %d is %s\n",result->pid,result->row+1,result->val);
                }
                else if(result->group==2){
                    printf("Validation result from process ID-%d: %d of 9 columns are valid\n",result->pid,result->totalValid);
                }
                else if(result->group==3){
                    printf("Validation result from process ID-%d: %d of 9 sub-grids are valid\n",result->pid,result->totalValid);
                }
            sem_post(&sems->counterLock);    
        sem_post(&sems->empty);
        
        if(i==10){
            sem_wait(&sems->counterLock);
            if(*ptrCounter < 27){
                printf("There are %d valid subgrids, and thus the solution is invalid.\n",*ptrCounter);
            }
            else{
                printf("There are %d valid subgrids, and thus the solution is valid.\n",*ptrCounter);
            }
            sem_post(&sems->counterLock);
        }
    }
    _exit(0);
}

/*FUNCTION: CreateProcesses
  OVERVIEW: Creates 11 processes to perform the calculations on the assigned regions
            of the sudoku grid, and then calls the consumer function which reads
            in the produced results from these child processes.*/

int createProcesses(){
    int i,j=0,k;
    pid_t par = getpid();
    pid_t child = 0;
    //Ensures no zombie processes can occur
    signal(SIGCHLD,SIG_IGN);
    for(i = 0;i<9;i++){
        rows[i].column=0;
        rows[i].row=i;
    }
    
    for(i = 0; i < 11; i++){ 		
        if(getpid() == par){ 
            k=i;
            child = fork();
            j++;
        }
    }
    
    if(child == 0 && j<10){
        checkRow(rows[k]);
    }
    else if(child == 0 && j==10)
    {
        checkColumns();
    }
    else if(child == 0 && j==11){
        checkSubGrids();
    }
    else{
        consumer();
    }
}    

/*FUNCTION: CreateMemory
  OVERVIEW: This function takes in the pointer to the integer reference values
            (descriptors) for the shared memory allocation. It uses the shm_open,
            ftruncate and mmap functions to allocate and establish the shared
            memory for the child processes to access.
            The 5 bits of shared memory are for buffer1 (the sudoku grid), buffer2
            (a results array), counter (the number of valid subgrids), sems (the
            semaphores used for this program) and result (which stores the
            information/results from each process to be passed to the consumer).*/

void createMemory(Descriptors* desc){
    desc->buffer1 = shm_open("Buffer1", O_CREAT | O_RDWR, 0666);
    ftruncate(desc->buffer1, bufferSize1);
    ptrBuffer1 = (int (*)[9])mmap(NULL,bufferSize1, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, desc->buffer1, 0);
    
    desc->buffer2 = shm_open("Buffer2", O_CREAT | O_RDWR, 0666);
    ftruncate(desc->buffer2, bufferSize2);
    ptrBuffer2 = (int*)mmap(NULL,bufferSize1, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, desc->buffer2, 0);
    
    desc->counter = shm_open("Counter", O_CREAT | O_RDWR, 0666);
    ftruncate(desc->counter, counterSize);
    ptrCounter = (int*)mmap(NULL,bufferSize1, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, desc->counter, 0);
    
    desc->sems = shm_open("Semaphores", O_CREAT | O_RDWR, 0666);
    ftruncate(desc->sems, semsSize);
    sems = (Semaphores*)mmap(NULL,semsSize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, desc->sems, 0);
    
    desc->result = shm_open("Result", O_CREAT | O_RDWR, 0666);
    ftruncate(desc->result, resultSize);
    result = (Result*)mmap(NULL,resultSize, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, desc->result, 0);
}

/*FUNCTION: RemoveMemory
  OVERVIEW: This function removes the shared memory utilised by the program by
            unmapping and unlinking the memory.*/

void removeMemory(){
    munmap(ptrBuffer1, bufferSize1);
    munmap(ptrBuffer2, bufferSize2);
    munmap(ptrCounter, counterSize);
    munmap(sems, semsSize);
    munmap(result,resultSize);
    
    shm_unlink("Buffer1");
    shm_unlink("Buffer2");
    shm_unlink("Counter");
    shm_unlink("Semaphores");
    shm_unlink("Result");
}

/*FUNCTION: CreateSemaphores
  OVERVIEW: Initialises the 4 semaphores used in the program. writeLock is a mutex
            for access to writing to the logfile, counterLock is a mutex to update
            the counter and empty/full are used for signalling to and from the
            consumer and producer processes (as per the producer-consumer model).*/

void createSemaphores(){
    sem_init(&sems->writeLock,1,1);
    sem_init(&sems->counterLock,1,1);
    sem_init(&sems->empty,1,1);
    sem_init(&sems->full,1,0);
}

/*FUNCTION: RemoveSemaphores
  OVERVIEW: Destroys the semaphores pointed to by the given memory addresses once
            the program has finished using them (i.e. at the end of the consumer
            process).*/

void removeSemaphores(){
    sem_destroy(&sems->writeLock);
    sem_destroy(&sems->counterLock);
    sem_destroy(&sems->empty);
    sem_destroy(&sems->full);
}

int main(int argc, char* argv[]){
    
    //Validate that the number of command line parameters is correct and that 
    //the maxDelay specified is non-negative and non-zero
    if(argc != 3){
        return 1;
    }
    if(atoi(argv[2]) <= 0){
        printf("Delay cannot be negative or zero!\n");
        return 1;
    }
    
    //Seed the random number generator
    srand(time(NULL));
    maxDelay = atoi(argv[2]);
    
    //Wipe the logfile for clean writing of errors
    writeFile = fopen(logfile,"w");
    fclose(writeFile);
    
    //Create and initialise memory
    Descriptors desc;
    createMemory(&desc);
    createSemaphores();
    initialiseSharedMem();
    
    //Read in the textfile to store the sudoku grid
    readFile(argv[1]);
    
    //Create the processes, run the calculations and output the result
    createProcesses();
    
    //Clean up memory and semaphores
    removeMemory();
    removeSemaphores();
    
    //Close the logfile
    fclose(writeFile);
    return 0;
}
