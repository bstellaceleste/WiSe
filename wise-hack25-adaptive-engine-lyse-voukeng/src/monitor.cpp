#include <iostream>
#include <cstdint>

// Mesure de l'activité de la pile
uintptr_t get_stack_usage() {
    uintptr_t rsp;
    asm volatile ("mov %%rsp, %0" : "=r"(rsp));
    return rsp;
}

// Logique de monitoring
void log_activity(const char* func_name, int count) {
    uintptr_t current_stack = get_stack_usage();
    std::cout << "[PHASE 1] Fonction : " << func_name 
              << " | Appels : " << count 
              << " | Stack Pointer : " << std::hex << current_stack << std::dec << std::endl;
}
