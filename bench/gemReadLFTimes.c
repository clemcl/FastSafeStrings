#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#define MAX_REC 512

int long long sink = 0;
 
int main() {
    // Opening the standard ASCII text file with newlines
    FILE *fp = fopen("race_test.txt", "r");
    if (!fp) {
        printf("Error: Could not open test file.\n");
        return 1;
    }

    char buffer[MAX_REC];
    char work_area[MAX_REC + 64];
    
    uint64_t records_processed = 0;      
    
    clock_t start = clock();
    
    // fgets must scan every byte for \n or \r\n
    while (fgets(buffer, MAX_REC, fp)) {
        
        // Pass 2: Find the newline to null-terminate (requires a scan)
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }

        // Pass 3: Standard concatenation (requires a scan to find destination end)
        strcpy(work_area, "PROC:");
        strcat(work_area, buffer); 
        sink += buffer[0];     
        records_processed++;     
    }
    
    clock_t end = clock();
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    printf("EOF Reached. Processed %llu records.\n", records_processed);
    printf("Standard C (fgets/strcat) Time: %f seconds\n", cpu_time_used);
           
    fclose(fp);
    return 0;
}
