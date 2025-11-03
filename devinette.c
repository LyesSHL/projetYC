// fun_guess.c — petit jeu animé en C (terminal)
// Compile:  gcc fun_guess.c -o fun_guess
// Run    :  ./fun_guess
// (Fonctionne sur macOS/Linux. Sur Windows 10+, ça marche dans le terminal moderne.)

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
  static void usleep_win(unsigned long usec) { Sleep((usec + 999)/1000); }
  #define usleep usleep_win
#else
  #include <unistd.h>
#endif

// Séquences ANSI
#define ANSI_RESET      "\x1b[0m"
#define ANSI_BOLD       "\x1b[1m"
#define ANSI_DIM        "\x1b[2m"
#define ANSI_CLEAR      "\x1b[2J\x1b[H"
#define ANSI_HIDE       "\x1b[?25l"
#define ANSI_SHOW       "\x1b[?25h"

// Couleurs 256 (avant-plan)
#define FG256(n)        "\x1b[38;5;" n "m"
#define BG256(n)        "\x1b[48;5;" n "m"

// Quelques couleurs rapides
#define C_PRIMARY   "\x1b[38;5;81m"    // bleu cyan
#define C_ACCENT    "\x1b[38;5;213m"   // rose
#define C_GOOD      "\x1b[38;5;82m"    // vert
#define C_WARN      "\x1b[38;5;214m"   // orange
#define C_BAD       "\x1b[38;5;196m"   // rouge
#define C_MUTED     "\x1b[38;5;245m"   // gris

static void type_print(const char* s, int us_per_char) {
    for (const char* p = s; *p; ++p) {
        putchar(*p);
        fflush(stdout);
        usleep(us_per_char);
    }
}

static void spinner(const char* label, int cycles, int delay_ms) {
    const char* frames = "|/-\\";
    printf("%s", label);
    for (int i = 0; i < cycles; ++i) {
        printf(" %c\r", frames[i % 4]);
        fflush(stdout);
        usleep(delay_ms * 1000);
        printf("\x1b[K"); // clear to EOL
    }
}

static int clamp(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// Barre “chaleur” 0..1 → dégradé + remplissage
static void heatbar(double ratio) {
    int width = 30;
    if (ratio < 0) ratio = 0;
    if (ratio > 1) ratio = 1;

    int fill = (int)(ratio * width + 0.5);
    // palette du froid (bleu 27) au chaud (rouge 196) en passant par jaune 226
    int cold = 27, mid = 226, hot = 196;
    // mélange simple : si ratio < 0.5 → cold→mid, sinon mid→hot
    int c1 = (ratio < 0.5) ? cold : mid;
    int c2 = (ratio < 0.5) ? mid  : hot;
    double t = (ratio < 0.5) ? (ratio/0.5) : ((ratio-0.5)/0.5);
    int col = (int)((1.0 - t)*c1 + t*c2);

    printf("Chaleur: " BG256("236"));
    for (int i = 0; i < width; ++i) {
        if (i < fill) printf("\x1b[48;5;%dm ", col);
        else          printf(BG256("236") " ");
    }
    printf(ANSI_RESET " ");
    if      (ratio > 0.8) printf(C_GOOD  "Très chaud 🔥" ANSI_RESET);
    else if (ratio > 0.6) printf(C_WARN  "Chaudière ♨️ " ANSI_RESET);
    else if (ratio > 0.4) printf(C_ACCENT"Tiède ✨" ANSI_RESET);
    else if (ratio > 0.2) printf(C_PRIMARY"Frais 💧" ANSI_RESET);
    else                  printf(C_MUTED "Glacial 🧊" ANSI_RESET);
    printf("\n");
}

static void banner(void) {
    printf(ANSI_CLEAR ANSI_HIDE);
    const char* lines[] = {
        "  ____             _         _   _                     ",
        " / ___| _   _  ___| | ____ _| |_| |__   ___  _ __ ___ ",
        " \\___ \\| | | |/ __| |/ / _` | __| '_ \\ / _ \\| '__/ _ \\",
        "  ___) | |_| | (__|   < (_| | |_| | | | (_) | | |  __/",
        " |____/ \\__,_|\\___|_|\\_\\__,_|\\__|_| |_|\\___/|_|  \\___|",
    };
    for (int i = 0; i < 5; ++i) {
        printf(C_PRIMARY ANSI_BOLD "%s\n" ANSI_RESET, lines[i]);
        usleep(16000);
    }
    printf("\n");
    type_print(C_MUTED "Devine le nombre secret entre " ANSI_RESET, 12000);
    type_print(C_ACCENT "1" ANSI_RESET, 40000);
    type_print(C_MUTED " et " ANSI_RESET, 12000);
    type_print(C_ACCENT "100" ANSI_RESET, 40000);
    type_print(C_MUTED "...\n\n" ANSI_RESET, 12000);
}

static void fireworks(int width, int height, int bursts, int frames, int delay_ms) {
    // petite anim de feu d’artifice ASCII
    for (int b = 0; b < bursts; ++b) {
        int cx = 5 + rand() % (width - 10);
        int cy = 3 + rand() % (height - 6);
        // montée
        for (int y = height - 2; y >= cy; --y) {
            printf("\x1b[s"); // save cursor
            printf("\x1b[%d;%dH", y, cx);
            printf(C_MUTED "•" ANSI_RESET);
            printf("\x1b[u"); // restore
            fflush(stdout);
            usleep(delay_ms * 800);
            // efface la précédente
            printf("\x1b[s");
            printf("\x1b[%d;%dH", y, cx);
            printf(" ");
            printf("\x1b[u");
        }
        // explosion
        for (int f = 0; f < frames; ++f) {
            int color = 196 + rand()%30; // rouges/roses/jaunes
            int radius = 1 + f;
            for (int k = 0; k < 12; ++k) {
                double ang = (3.14159*2.0/12.0)*k;
                int x = cx + (int)(radius * 1.5 * cos(ang));
                int y = cy + (int)(radius * 0.9 * sin(ang));
                printf("\x1b[s");
                printf("\x1b[%d;%dH", y, x);
                printf("\x1b[38;5;%dm*\x1b[0m", color);
                printf("\x1b[u");
            }
            fflush(stdout);
            usleep(delay_ms * 1000);
        }
    }
    printf("\n");
}

int main(void) {
    // seed RNG
    srand((unsigned int)time(NULL));

    // (facultatif) activer ANSI sur vieux Windows
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            mode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
            SetConsoleMode(hOut, mode);
        }
    }
#endif

    banner();

    int secret = 1 + rand() % 100;
    int attempts = 0;
    int best_diff = 1000;

    while (1) {
        printf(ANSI_BOLD C_PRIMARY "➡︎ Entre un nombre: " ANSI_RESET);
        fflush(stdout);

        char buf[64];
        if (!fgets(buf, sizeof(buf), stdin)) {
            printf("\n" C_BAD "Entrée invalide." ANSI_RESET "\n");
            break;
        }

        // enlever le \n
        buf[strcspn(buf, "\r\n")] = 0;

        // vérifier nombre
        char *end = NULL;
        long val = strtol(buf, &end, 10);
        if (end == buf || *end != '\0' || val < 1 || val > 100) {
            printf(C_WARN "Entre un entier entre 1 et 100 😉" ANSI_RESET "\n\n");
            continue;
        }

        int guess = (int)val;
        attempts++;

        spinner(C_MUTED "Vérification", 10, 28);

        int diff = abs(guess - secret);
        if (diff < best_diff) best_diff = diff;

        // Affichage feedback
        if (guess == secret) {
            printf("\r\x1b[K"); // clear line
            printf(C_GOOD ANSI_BOLD "✅ Bravo ! " ANSI_RESET);
            printf("Tu as trouvé en %d tentative%s.\n\n", attempts, attempts>1?"s":"");

            // mini feu d’artifice
            fireworks(80, 24, 3, 6, 30);

            // message final
            printf(ANSI_BOLD C_ACCENT "Le nombre secret était bien %d 🎯\n" ANSI_RESET, secret);
            printf(C_MUTED "Appuie sur Entrée pour quitter..." ANSI_RESET);
            fflush(stdout);
            (void)fgets(buf, sizeof(buf), stdin);
            printf(ANSI_SHOW ANSI_RESET "\n");
            return 0;
        } else {
            printf("\r\x1b[K"); // clear line
            if (guess < secret) {
                printf(C_ACCENT "C'est plus grand ⬆️" ANSI_RESET "\n");
            } else {
                printf(C_ACCENT "C'est plus petit ⬇️" ANSI_RESET "\n");
            }

            // ratio de chaleur basé sur le meilleur écart vu
            double ratio = 1.0 - (best_diff / 100.0);
            heatbar(ratio);

            // “indice” fun
            if (diff <= 5) {
                printf(C_GOOD "Tu brûles ! Presque ! 🔥\n\n" ANSI_RESET);
            } else if (diff <= 10) {
                printf(C_WARN "Tu chauffes… ♨️\n\n" ANSI_RESET);
            } else if (diff <= 20) {
                printf(C_PRIMARY "Tu te rapproches 🌡️\n\n" ANSI_RESET);
            } else {
                printf(C_MUTED "Brr… c’est froid 🧊\n\n" ANSI_RESET);
            }
        }
    }

    printf(ANSI_SHOW ANSI_RESET);
    return 0;
}

