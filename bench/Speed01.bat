gcc -O3 -I..\   ..\vbx_*.c Makefile.c -o Makefile 2>&1 | more 
gcc -O3 -I..\   ..\vbx_*.c ReadFgets.c -o  ReadFgets 2>&1 | more 
gcc -O3 -I..\   ..\vbx_*.c ReadVBF.c -o  ReadVBF 2>&1 | more 
Echo Making Test Files
MakeFile
Echo FGETS Times
ReadFgets
Echo Variable Blocked and FAST Strings Times
ReadVBF
