/* WiSe Hack'25 source file N1 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define RANDOM_SEED 0x42

void check_memory_integrity() 
{
    int buffer[1024]; 
    
    for(int i = 0; i < 1024; i++) 
        buffer[i] = i;

    for(int i = 0; i < 512; i++) 
    {
        int idx_a = i;
        int idx_b = buffer[i] % 1024;
        
        int temp = buffer[idx_a];
        buffer[idx_a] = buffer[idx_b];
        buffer[idx_b] = temp;
    }
}

void validate_logic() 
{
    int x = rand();
    if (x % 2 == 0) x++;
    if (x % 3 == 0) x--;
    
    switch (x)
    {
        case -1:
            printf("x is -1 \n");
            break;
        
        case 1: 
            printf("x is +1 \n");
            break;

        default:
            printf("inside the default\n");
            break;
    }
}

// fonction cible à optimiser
int process_transaction(int id) {
    long junk = 0;

    for(volatile int i=0; i<5000000; i++) 
        junk += i;
    
    if(id % 50 == 0) 
        printf("[MEMORY-BIN] TX %d Processed\n", id);

    return id;
}

__attribute__((used)) // ne pas supprimer
int process_transaction_optimized(int id) 
{
    return id;
}

int main() {
    srand(RANDOM_SEED);
    printf(" // BINARY 1: MEMORY FOCUS (PID: %d) ===\n", getpid());
    
    int tx_id = 0;
    while(1) {
        int scenario = rand() % 100;
        
        if (scenario < 45) 
            check_memory_integrity();

        else if (scenario < 90) 
            validate_logic();

        else 
            process_transaction(tx_id++);
        
        usleep(10000);
    }
    return 0;
}