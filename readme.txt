Lisa Batbouta labatbouta@wpi.edu
cs 3013
Project 4
 
This project allows a user to compare standard file I/O of read() verses mmap. It also allows the user to parallize the process of calculating the statistics of maximum string value and string lengths greater than 4. The user can enter 2 or 3 arguements. The user can enter the chunk size to read the file in and/or  calculate the statistics in memory, or the number of threads to distribute the work over, or to map the process to memory. If given 2 arguements then the default chunk size is set to 1024.
Aside from the proj.c file there is the doit.c file from project works with the proj.c file for the analysis particularly focusing on major page faults and wall-clock time. There is a report within this zip that reports the finds from using doit.c with proj.c.

