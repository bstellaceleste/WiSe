/* WiSe Hack'25 source file N3 - BALANCED / REALISTIC */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h> 

#define RANDOM_SEED 0x16

void check_memory_integrity() 
{
    char buffer[128];
    sprintf(buffer, "Check-%d", rand());

    volatile int x = buffer[64];
    x++;
    buffer[65] == x;
    (void)x;

    volatile int y = ++x;
    for (volatile int i = 0; i < 128; ++i)
        buffer[i] = ++x;

    (void)y;
}

void validate_logic() 
{
    // Logique standard
    int x = rand();
    if (x % 5 == 0) return;
}

// Fonction cible à optimiser
int process_transaction(int id) {
    double tax = 0;

    // Simulation d'un calcul de taxe inefficace
    for(volatile int i=0; i<20000; i++) {
        tax += sqrt((double)i * id); // Opération coûteuse
    }
    
    if(id % 50 == 0) 
        printf("[MOMO-BIN] TX %d Processed (Val: %.2f)\n", id, tax);

    return id;
}

// Version optimisée (Cible de la redirection)
__attribute__((used)) 
int process_transaction_optimized(int id)
{
    return id + id*0.2; 
}

int main() {
    srand(RANDOM_SEED);
    printf(" // BINARY 3: BALANCED MOMO (PID: %d) ===\n", getpid());
    
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