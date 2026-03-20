gcc -O3 -I..\   ..\vb_*.c GemCreateTestFiles.c -o GemCreateTestFiles 2>&1 | more 
gcc -O3 -I..\   ..\vb_*.c  gemReadLFTimes.c -o  gemReadLFTimes 2>&1 | more 
gcc -O3 -I..\   ..\vb_*.c  gemReadVBTimes.c -o  GemReadVBTimes 2>&1 | more 
GemCreateTestFiles
Echo FGETS Times
gemReadLFTimes
Echo Variable Blocked and FAST Strings Times
gemReadVBTimes
