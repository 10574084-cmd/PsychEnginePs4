#include <stdio.h>
#include <unistd.h>

/* Native OpenOrbis smoke test only; the gameplay engine is not integrated yet. */
int main(void) {
    puts("PsychEnginePs4 bootstrap loaded.");
    puts("Native PS4 toolchain/package smoke test.");
    puts("This is not the playable Psych Engine game.");
    fflush(stdout);
    sleep(2);
    return 0;
}
