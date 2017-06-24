**************************************************************
*       MSSV - Jeremy Ciccarelli - 18760376 - 08/05/2017     *                                                      
**************************************************************

This assignment submission file is separated into two folders, AssignmentProcess and AssignmentThread, which corresponds to the implementation of the sudoku solution validator using multiple processes and threads respectively.
As such, there are separate Makefiles for each of the programs. In order to compile, navigate the terminal to the directory of the program to be run, and type 'make' in order to compile.
A 'make clean' option is provided and recommended to be run prior to compiling.

In order to execute either program from the command line in the terminal, use the following:

./mssv [input file name] [maxDelay]

where input file name refers to the filename of the sudoku grid to be validated (should be a .txt file) and the maxDelay represents a positive integer which is used in sleep() calls within the program.