There are two programs to show the speed and cpu differences between
standard C routines and the Fast Safe Strings and Fast Input/Output routines.
 
This file discusses the fast VB File IO. See AAStringResults.txt for  
the internal Fast safe string results and speeds, 
 
Running the Speed01 Bat file will compile some programs to:
1. Create some testfiles.  The first is a text file, the second a VB fast
   file.
2. A program ReadFgets to read the text file using Fgets.
3. A program ReadVBF to read the special VB file.
 
These program read the files and display the CPU time to read them.
Typically on Windows using GCC, the fast string code is 7 times faster
than the standard C code.
 
In this run, the program output is:
 
   EOF Reached. Processed 5000000 records.
   Standard C (fgets/strcat) Time: 1.448000 seconds
   Challenger (FSS Logic) Time: 0.189000 seconds
    
In other words, on this Lenovo I7 the fast routines are 7.6 times
faster.
 
----------
 
Running the Speed02 Bat file will compile some programs to 
read and write updated files created by Speed01 above. 
1. Program UpdateFgets reads the file with FGETS, and adds some data .
   to each line, and writes the updated file. 
3. Program UpdateVB does the same using the high speed record IO.
   and Fast String routines to do as 1. above.
 
These program read and write files and display the CPU time to do this..
Typically on Windows using GCC, the fast string code is 12 times faster
than the standard C code.
 
In this run, the program output is:
 
   Total Matches Found: 5000000.
   Standard C (fgets/strstr/fprintf) Time: 9.249000 seconds
   Challenger Time: 0.719000 seconds 
    
In other words, on this Lenovo I7 the fast routines are 12.8 times
faster.
 
