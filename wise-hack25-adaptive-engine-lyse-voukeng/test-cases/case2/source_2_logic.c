/* WiSe Hack'25 source file N2 - LOGIC FOCUS */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define RANDOM_SEED 0x33

void check_memory_integrity() 
{
    // Version allégée pour ce binaire
    volatile int x = 0;
    --x;
    ++x;

    volatile int y = ++x;
    y--;
    
    volatile int z = x + y;
    (void)z;
}

void validate_logic() 
{
    int x = rand() % 100;
    int res = 0;

    if (x < 50) {
        if (x % 2 == 0) {
            switch(x % 4) {
                case 0: res = 1; break;
                case 1: res = 2; break;
                default: res = 3; break;
            }
        } else {
            for(int i=0; i<10; i++) res += i;
        }
    } else {
        if (x > 80) res = 5;
        else if (x > 70) res = 6;
        else res = 7;
    }
    
    volatile int keep = res; 
    (void)keep;
}

// Fonction cible à optimiser
int process_transaction(int id) {
    long junk = 0;
    // Boucle CPU pure
    for(volatile int i=0; i<5000000; i++) 
        junk += i;
    
    if(id % 50 == 0) 
        printf("[LOGIC-BIN] TX %d Processed\n", id);

    return id;
}

// Version optimisée (Cible de la redirection)
__attribute__((used)) 
int process_transaction_optimized(int id)
{
    return id; 
}

int main() {
    srand(RANDOM_SEED);
    printf(" // BINARY 2: LOGIC FOCUS (PID: %d) ===\n", getpid());
    
    int tx_id = 0;
    while(1) {
        int scenario = rand() % 100;
        
        if (scenario < 45) check_memory_integrity();
        else if (scenario < 90) validate_logic();
        else process_transaction(tx_id++);
        
        usleep(10000);
    }
    return 0;
}