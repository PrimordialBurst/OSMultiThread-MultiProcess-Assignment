
/*Name: Jeremy Ciccarelli
  Student ID: 18760376
  Program: Multi-threaded Sudoku Solution Validator
  Overview: This program reads in a provided sudoku solution in a textfile
            and determines whether or not it is valid, by creating 11 threads
            to validate the 27 separate subgrids of the solution (9 rows,
            9 columns, 9 3x3 subgrids). If all 27 subgrids are valid, then the
            solution will be deemed valid. If any errors are detected within
            the solution, they will be written to the logfile which is created
            at runtime.*/

/*Imports*/
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "structs.h"


/*Declarations of variables*/
const char* logfile = "logfile.txt";

pthread_mutex_t mutex;
pthread_cond_t empty;
pthread_cond_t full;
FILE* writeFile;
Result* result;

int counter = 0;
int solutionValid[11] = {0};
int grid[9][9];
int buffer = 0;
int maxDelay;
Parameters** param;

/*FUNCTION: CheckRow
  OVERVIEW: This function takes in an assigned row of a sudoku grid, checks to make
            sure it's a valid row (contains uniquely numbers 1-9) and passes
            the result (as a theoretical 'producer') to the consumer function
            upon requiring the mutex lock. If a row is deemed invalid, it will
            be written to the logfile.*/

void* checkRow(void* parameter){
    
    Parameters* param = parameter;
    int i,num,valid=0;
    int row = param->row;
    
    int usedArray[9] = {0};
    for(i=0;i<9;i++){
        num = grid[row][i];
        if((num < 1)|| (num >9) || usedArray[num-1] !=0){
            valid =1;
        }
        else{
           usedArray[num-1] = 1;
        }
    }
    
    sleep(rand() % maxDelay);
    
    solutionValid[row] = 1-valid;
    //Acquire mutex
    pthread_mutex_lock(&mutex);
        //Wait until the buffer is empty
        while(buffer != 0){
                pthread_cond_wait(&empty,&mutex);
            }
            //Update the result struct with the row, thread ID etc.
            result->row = row;
            result->tid = pthread_self();
            result->group = 1;
            if(valid == 0){
               result->val = "valid";
               counter = counter+1;
            }
            else{
                //Error detected - write to logfile
                result->val = "invalid";
                writeFile = fopen(logfile,"a");
                fprintf(writeFile,"thread ID-%lu: row %d is invalid\n",pthread_self(),row+1);
                fclose(writeFile);
            }
            buffer++;
        pthread_cond_signal(&full);
    //Release mutex
    pthread_mutex_unlock(&mutex);
    
    pthread_exit(0);
 }

/*FUNCTION: CheckColumns
  OVERVIEW: This function checks over each column in the sudoku grid, validating
            each one and maintaining a count (totalValid) which is then passed to
            the consumer process via the result struct. Additionally, any errors
            detected are written to the logfile once the mutex is acquired.*/

void* checkColumns(){
    int i,j,num,curValid,totalValid=0;
    char* log = (char*)calloc(9*257,sizeof(char));
    char* line = (char*)calloc(257,sizeof(char));
    for(i=0;i<9;i++){
        curValid = 0;
        int usedArray[9] = {0};
        for(j=0;j<9;j++){
            num = grid[j][i];
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
            sprintf(line,"thread ID-%lu: column %d is invalid\n",pthread_self(),i+1);
            strncat(log,line,257);
        }
    }
    sleep(rand() % maxDelay);
    
    solutionValid[9] = totalValid;
    //Acquire mutex
    pthread_mutex_lock(&mutex);
        while(buffer != 0){
                pthread_cond_wait(&empty,&mutex);
            }
            counter = counter+totalValid;
            result->tid = pthread_self();
            result->group = 2;
            result->totalValid = totalValid;
            writeFile = fopen(logfile,"a");
            fputs(log,writeFile);
            fclose(writeFile);
            buffer++;
        pthread_cond_signal(&full);
    //Release mutex    
    pthread_mutex_unlock(&mutex);
    
    free(log);
    free(line);
    pthread_exit(0);
}


/*FUNCTION: CheckSubGrids
  OVERVIEW: This function iterates through the 9 3x3 subgrids of the sudoku grid,
            validating each square. Maintains a count of valid subgrids and appends
            error messages detected onto a string which is written to the logfile
            once the mutex is acquired and results are passed to the consumer thread.*/

void* checkSubGrids(){
    int i,j,k,l,num,curValid,totalValid=0;
    
    //To be written to logfile
    char* log = (char*)calloc(9*257,sizeof(char));
    char* line = (char*)calloc(257,sizeof(char));
    for(i=0;i<9;i= i+3){
        for(j=0;j<9;j=j+3){
            //SUB GRID START
            curValid = 0;
            int usedArray[9] = {0};
            for(k=i;k<i+3;k++){
                for(l=j;l<j+3;l++){
                    num = grid[k][l];
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
                sprintf(line,"thread ID-%lu: subgrid [%d..%d, %d..%d] is invalid\n",pthread_self(),i+1,i+3,j+1,j+3);
                strncat(log,line,257);
            }
        }
    }
    sleep(rand() % maxDelay);
    
    solutionValid[10] = totalValid;
    //Acquire mutex
    pthread_mutex_lock(&mutex);
        while(buffer != 0){
                pthread_cond_wait(&empty,&mutex);
            }
            counter = counter+totalValid;
            result->tid = pthread_self();
            result->group = 3;
            result->totalValid = totalValid;
            
            //Write to log file
            writeFile = fopen(logfile,"a");
            fputs(log,writeFile);
            fclose(writeFile);
            
            buffer++;
        pthread_cond_signal(&full);
    //Release mutex    
    pthread_mutex_unlock(&mutex);
    
    //Free memory
    free(log);
    free(line);
    //Terminate thread
    pthread_exit(0);
}
/*FUNCTION: Consumer
  OVERVIEW: This function performs 11 iterations (corresponding to the 11 calculation
            threads) of waiting for a result to be produced by one of these such threads.
            It prints out the results of each validation, taken from the result struct,
            and at the end of the 11th thread, it checks to see if the counter for
            the number of valid subgrids. If counter is equal to 27, it will indicate
            that the sudoku grid is valid, otherwise deem it invalid. */

void* consumer(){
    int i;
    for(i =0;i<11;i++){
        pthread_mutex_lock(&mutex);
            while(buffer == 0){
                pthread_cond_wait(&full,&mutex);
            }
                if(result->group == 1){
                    printf("Validation result from thread ID-%lu: row %d is %s\n",result->tid,result->row+1,result->val);
                }
                else if(result->group==2){
                    printf("Validation result from thread ID-%lu: %d of 9 columns are valid\n",result->tid,result->totalValid);
                }
                else if(result->group==3){
                    printf("Validation result from thread ID-%lu: %d of 9 sub-grids are valid\n",result->tid,result->totalValid);
                }
                
                buffer--;
            pthread_cond_signal(&empty);    
            pthread_mutex_unlock(&mutex);
        
        /*Once all threads have been read in by the consumer thread, check
          the value of the counter and print out the validity of the solution grid*/
        if(i==10){
            pthread_mutex_lock(&mutex);
            if(counter < 27){
                printf("There are %d valid subgrids, and thus the solution is invalid.\n",counter);
            }
            else{
                printf("There are %d valid subgrids, and thus the solution is valid.\n",counter);
            }
            pthread_mutex_unlock(&mutex);
        }
    }
    
    pthread_exit(0);
}

/*FUNCTION: CreateThreads
  OVERVIEW: Creates 11 threads to perform the calculations on the assigned regions
            of the sudoku grid, as well as a 'consumer' thread which reads
            in the produced results from these threads.
            pthread_join is used in order to suspend the execution of this thread
            (the calling one) whilst the other threads are still in execution. */

int createThreads(){
    pthread_t threads[11];
    pthread_t consumerThread;
    int i;
    //Create the 9 threads to validate the rows
    for(i=0;i<9;i++){
        param[i]->row = i;
        param[i]->column = 0;
        pthread_create(&threads[i],NULL,checkRow,param[i]);
    }
    //Create thread to validate the columns
    pthread_create(&threads[9],NULL,checkColumns,NULL);
    //Create thread to validate the subgrids
    pthread_create(&threads[10],NULL,checkSubGrids,NULL);
    //Create thread to read in all the results
    pthread_create(&consumerThread,NULL,consumer,NULL);
    
    //Suspend execution of this thread until all of the other threads have finished
    for(i = 0;i<11;i++){
        pthread_join(threads[i], NULL);
    }
    pthread_join(consumerThread,NULL);
    return 0;
}

/*FUNCTION: ReadFile
  OVERVIEW: This function performs the reading of the specified .txt file specified
            as a command line parameter, assigning it to the 2D array 'grid'.*/

int readFile(char* filename){
    int i = 0;
    int j = 0;
    int retVal =0;
    FILE* readFile;
    
    readFile = fopen(filename,"r");
    //Check for reading errors
    if(readFile == NULL){
        printf("Error reading file");
    }
    else{
        while(!feof(readFile) && i<9){
            fscanf(readFile, "%1d",&grid[i][j]);
            j++;
            if(j>8){
                i++;
                j=0;
            }
        }
        retVal = 1;
    }
    fclose(readFile);
    return retVal;
}

int main(int argc, char* argv[]){
    
    /*Check number of command line parameters is correct*/
    if(argc != 3){
        return 1;
    }
    srand(time(NULL));
    maxDelay = atoi(argv[2]);
    
    result = (Result*)malloc(sizeof(Result));
    param = (Parameters**)malloc(9*sizeof(Parameters*));
    int i;
    for(i = 0;i<9;i++){
        param[i] = (Parameters*)malloc(sizeof(Parameters));
    } 
    /*Read in the textfile representation of the sudoku grid*/
    readFile(argv[1]);
    
    /*Clear the logfile for writing of new errors (if applicable)*/
    writeFile = fopen(logfile,"w");
    fclose(writeFile);
    
    /*Initialise the mutex and thread condition variables*/
    pthread_mutex_init(&mutex, NULL );
    pthread_cond_init(&full, NULL );
    pthread_cond_init(&empty, NULL );
    
    /*Create the threads, which perform the necessary calculations for the program*/
    createThreads();
    
    /*Destroy the locks*/
    pthread_mutex_destroy(&mutex );
    pthread_cond_destroy(&full );
    pthread_cond_destroy(&empty );
    
    /*Free the memory and return successfully*/
    for(i = 0;i<9;i++){
        free(param[i]);
    } 
    free(result);
    free(param);
    return 0;
}

