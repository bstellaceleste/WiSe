#include <sys/mman.h>
#include <unistd.h>
#include <cstring>

void apply_hotpatch(void* target, void* replacement) {
    size_t pagesize = sysconf(_SC_PAGESIZE);
    void* page = (void*)((uintptr_t)target & ~(pagesize - 1));

    // Déverrouillage pour écriture (Critère : Sécurité et Stabilité)
    mprotect(page, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC);

    // Opcode x86_64 : movabs rax, <addr>; jmp rax
    unsigned char patch[12] = {0x48, 0xB8}; 
    memcpy(&patch[2], &replacement, 8);
    patch[10] = 0xFF; patch[11] = 0xE0;

    memcpy(target, patch, 12); // Écriture directe en mémoire

    // Relock
    mprotect(page, pagesize, PROT_READ | PROT_EXEC);
}
