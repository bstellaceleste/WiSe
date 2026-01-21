/* WiSe Hack'25 source file N4 - MYSTERY / ADAPTIVE BOSS */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define RANDOM_SEED 0x42

// VARIABLES GLOBALES CACHÉES (État du Système)
// 0 = Mode Nominal (Rapide)
// 1 = Mode Crise (Lent / Surchauffe)
int SYSTEM_STATE = 0; 
int STATE_COUNTER = 0;

void check_memory_integrity() 
{
    // En mode crise, on simule aussi une légère fuite mémoire pour brouiller les pistes
    // mais pas assez pour déclencher le détecteur du Binaire 1
    if (SYSTEM_STATE == 1) {
        volatile int leak[100];
        leak[0] = 1; 
    } else {
        volatile int x = 0; x++;
    }
}

void validate_logic() 
{
    // --- PARTIE 1 : LE MÉCANISME DE CHANGEMENT D'ÉTAT (OBLIGATOIRE) ---
    // C'est ce qui rend le binaire "Vivant"
    STATE_COUNTER++;

    if (STATE_COUNTER % 300 == 0) {
        SYSTEM_STATE = !SYSTEM_STATE; // Bascule 0 <-> 1
        printf("\n>>> [SYSTEM ALERT] CHANGING OPERATING MODE TO: %s <<<\n\n", 
               SYSTEM_STATE ? "CRITICAL (SLOW)" : "NOMINAL (FAST)");
    }

    // --- PARTIE 2 : LE BROUILLAGE DE PISTES (COMPLEXITÉ) ---
    // Cette section génère un graphe de contrôle de flux (CFG) complexe.
    // Elle ne fait rien d'utile, mais elle est difficile à prédire pour un CPU.

    int input = rand();
    volatile int checksum = 0x12345678;
    int rounds = (input & 0x07) + 1; // Entre 1 et 8 tours

    // Point d'entrée aléatoire dans le code (Jump Table)
    switch (input & 0x03) { 
        case 0: goto NODE_ALPHA;
        case 1: goto NODE_BETA;
        case 2: checksum ^= 0xFFFF;
        case 3: goto NODE_GAMMA;
    }

NODE_ALPHA:
    // Simulation d'un calcul de hash CRC basique
    for (int i = 0; i < rounds; i++) {
        if (checksum & 1) 
            checksum = (checksum >> 1) ^ 0xA001; // Polynôme
        else 
            checksum = checksum >> 1;
        
        // Saut croisé conditionnel (Casse la linéarité)
        if (i == 3 && (input % 2 == 0)) goto NODE_GAMMA;
    }
    goto NODE_END;

NODE_BETA:
    // Boucle do-while avec condition de remontée
    do {
        checksum += 0x11;
        rounds--;
        // Remonte vers Alpha sous condition bitwise
        if ((checksum & 0xF0) == 0) goto NODE_ALPHA; 
    } while (rounds > 0);
    goto NODE_END;

NODE_GAMMA:
    // Opération dépendant de l'état global (Lien subtil avec le state machine)
    checksum = (checksum << 4) | (checksum >> 28); // Rotation bitwise
    if (SYSTEM_STATE == 1) checksum ^= 0xDEADBEEF;

NODE_END:
    (void)checksum; // Évite le warning "unused variable"
}

// FONCTION CIBLE À OPTIMISER (Comportement Hybride)
int process_transaction(int id) {
    
    // CAS 1 : MODE NOMINAL (Majorité du temps au début)
    // La fonction est saine et rapide.
    // Si le moteur remplace ça trop tôt, c'est de l'over-engineering.
    if (SYSTEM_STATE == 0) {
        for(volatile int i=0; i<100; i++); // Très rapide
    }
    
    // CAS 2 : MODE CRISE (Le Hotspot apparaît soudainement)
    // C'est ICI que le moteur doit intervenir dynamiquement.
    else {
        long junk = 0;
        // Boucle extrêmement lourde (Simule un timeout ou un algo de secours pourri)
        for(volatile int i=0; i<150000000; i++) 
            junk += i;
        
        // Log d'alerte
        if(id % 10 == 0) 
            printf("[MYSTERY] !!! HEAVY LOAD DETECTED ON TX %d !!!\n", id);
    }

    return id;
}

// VERSION OPTIMISÉE (La roue de secours)
// Votre moteur doit rediriger le flux ici quand SYSTEM_STATE vaut 1
__attribute__((used)) 
int process_transaction_optimized(int id)
{
    printf(">> EXECUTING THE OPTIMIZED PROCESSING FUNCTION (DYNAMICALLY INJECTED) <<\n");
    // On court-circuite tout le traitement lent
    return id; 
}

int main() {
    srand(RANDOM_SEED);
    printf(" // BINARY 4: MYSTERY BOSS (PID: %d) ===\n", getpid());
    printf(" // HINT: Watch the system behavior evolve...\n");
    
    int tx_id = 0;
    while(1) {
        int scenario = rand() % 100;
        
        if (scenario < 45) 
            check_memory_integrity();

        else if (scenario < 90) 
            validate_logic(); // Fait avancer le compteur d'état

        else 
            process_transaction(tx_id++);
        
        // Variation de la cadence d'appel
        // En mode crise (1), le main boucle plus vite pour accentuer la pression
        if (SYSTEM_STATE == 0) usleep(10000); // 10ms (Calme)
        else usleep(2000);                    // 2ms (Panique)
    }
    return 0;
}