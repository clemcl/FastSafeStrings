gcc -O3 -I..\  UpdateVB.c ..\vbx_*.c -o UpdateVB.exe 2>&1 | more 
gcc -O3 -I..\  UpdateFgets.c ..\vbx_*.c -o UpdateFgets.exe 2>&1 | more 
Echo Running Update Test Files
Echo FGETS Update Times
UpdateFgets.exe
Echo Variable Blocked and FAST Strings Times
UpdateVB.exe
