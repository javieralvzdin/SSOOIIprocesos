#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "parking2.h"

typedef int  (*T_P2_inicio)(TIPO_FUNCION_LLEGADA*, TIPO_FUNCION_SALIDA*, long, int);
typedef int  (*T_P2_fin)();
typedef int  (*T_P2_aparcar)(HCoche, void*, TIPO_FUNCION_APARCAR_COMMIT, TIPO_FUNCION_PERMISO_AVANCE, TIPO_FUNCION_PERMISO_AVANCE_COMMIT);
typedef int  (*T_P2_desaparcar)(HCoche, void*, TIPO_FUNCION_PERMISO_AVANCE, TIPO_FUNCION_PERMISO_AVANCE_COMMIT);
typedef int  (*T_P2_getNumero)(HCoche);
typedef int  (*T_P2_getLongitud)(HCoche);
typedef int  (*T_P2_getPosAcera)(HCoche);
typedef int  (*T_P2_getX2)(HCoche);
typedef int  (*T_P2_getY2)(HCoche);
typedef int  (*T_P2_getAlgoritmo)(HCoche);
typedef void* (*T_P2_getDatos)(HCoche);
typedef int  (*T_P2_getX)(HCoche);
typedef int  (*T_P2_getY)(HCoche);      

T_P2_inicio pP2_inicio; T_P2_fin pP2_fin; T_P2_aparcar pP2_aparcar;
T_P2_desaparcar pP2_desaparcar; T_P2_getNumero pP2_getNumero;
T_P2_getLongitud pP2_getLongitud; T_P2_getPosAcera pP2_getPosAcera;
T_P2_getAlgoritmo pP2_getAlgoritmo; T_P2_getDatos pP2_getDatos;
T_P2_getX2 pP2_getX2; T_P2_getY2 pP2_getY2; T_P2_getX pP2_getX; T_P2_getY pP2_getY;         

#define LONGITUD_ACERA 80
int aceras[4][LONGITUD_ACERA];
int carril[4][LONGITUD_ACERA];
int turno_actual[4] = { 1, 1, 1, 1 };
int ult_pos_siguiente[4] = { 0, 0, 0, 0 };

HANDLE hMutexAcera[4];
HANDLE hEventoTurno[4];
HANDLE hEventoCarril[4];

void EsperaSegura(HANDLE h, const char* msg) {
    if (WaitForSingleObject(h, INFINITE) == WAIT_FAILED) PERROR((char*)msg);
}

void LiberacionSegura(HANDLE h, const char* msg) {
    if (!ReleaseMutex(h)) PERROR((char*)msg);
}

void MiConfirmacionAparcar(HCoche hc) {
    int alg = pP2_getAlgoritmo(hc);
    EsperaSegura(hMutexAcera[alg], "Mutex ConfAparcar");
    turno_actual[alg]++;
    LiberacionSegura(hMutexAcera[alg], "Release ConfAparcar");
    if (!SetEvent(hEventoTurno[alg])) PERROR("Fallo SetEvent Turno");
}

void MiPermisoAvance(HCoche hc) {
    int alg = pP2_getAlgoritmo(hc);
    int x2 = pP2_getX2(hc), y2 = pP2_getY2(hc), lon = pP2_getLongitud(hc), num = pP2_getNumero(hc);
    int i, seguro;

    while (1) {
        EsperaSegura(hMutexAcera[alg], "Mutex PermisoAvance");
        seguro = 1;
        if (y2 == 2) {
            for (i = x2; i < x2 + lon && i < LONGITUD_ACERA; i++) {
                if (i >= 0 && carril[alg][i] != 0 && carril[alg][i] != num) { seguro = 0; break; }
            }
        }
        if (seguro) {
            if (y2 == 2) {
                for (i = x2; i < x2 + lon && i < LONGITUD_ACERA; i++) if (i >= 0) carril[alg][i] = num;
                if (!ResetEvent(hEventoCarril[alg])) PERROR("Fallo ResetEvent Carril");
            }
            LiberacionSegura(hMutexAcera[alg], "Avance OK");
            break;
        }
        if (SignalObjectAndWait(hMutexAcera[alg], hEventoCarril[alg], INFINITE, FALSE) == WAIT_FAILED) PERROR("Fallo Signal Carril");
    }
}

void MiConfirmacionAvance(HCoche hc) {
    int alg = pP2_getAlgoritmo(hc);
    int x = pP2_getX(hc), y = pP2_getY(hc), y2 = pP2_getY2(hc);
    int lon = pP2_getLongitud(hc), num = pP2_getNumero(hc), i;

    EsperaSegura(hMutexAcera[alg], "Mutex ConfAvance");
    for (i = 0; i < LONGITUD_ACERA; i++) {
        if (carril[alg][i] == num) {
            if (y != 2 || i < x || i >= x + lon) carril[alg][i] = 0;
        }
    }
    if (y == 2) {
        for (i = x; i < x + lon && i < LONGITUD_ACERA; i++) if (i >= 0) carril[alg][i] = num;
    }

    int pos_acera = (int)(intptr_t)pP2_getDatos(hc);
    if (y == 2 && y2 < 2 && pos_acera != -1) {
        for (i = 0; i < lon; i++) aceras[alg][pos_acera + i] = 0;
    }

    if (!SetEvent(hEventoCarril[alg])) PERROR("Fallo SetEvent Carril");
    LiberacionSegura(hMutexAcera[alg], "Release ConfAvance");
}

DWORD WINAPI HiloAparcar(LPVOID lpParam) {
    HCoche hc = (HCoche)lpParam;
    int alg = pP2_getAlgoritmo(hc), num = pP2_getNumero(hc);

    while (1) {
        EsperaSegura(hMutexAcera[alg], "Mutex HiloAparcar");
        if (turno_actual[alg] == num) { LiberacionSegura(hMutexAcera[alg], "Turno OK"); break; }

        if (SignalObjectAndWait(hMutexAcera[alg], hEventoTurno[alg], INFINITE, FALSE) == WAIT_FAILED) PERROR("Fallo Signal Turno");

        EsperaSegura(hMutexAcera[alg], "Recheck");
        if (turno_actual[alg] != num) {
            if (!SetEvent(hEventoTurno[alg])) PERROR("Fallo DaisyChain");
            LiberacionSegura(hMutexAcera[alg], "DaisyChain OK");
        }
        else { LiberacionSegura(hMutexAcera[alg], "Turno Pillado"); break; }
    }

    if (pP2_aparcar(hc, (void*)(intptr_t)pP2_getPosAcera(hc), MiConfirmacionAparcar, MiPermisoAvance, MiConfirmacionAvance) == -1)
        fprintf(stderr, "Error pP2_aparcar %d\n", num);
    return 0;
}

DWORD WINAPI HiloDesaparcar(LPVOID lpParam) {
    if (pP2_desaparcar((HCoche)lpParam, pP2_getDatos((HCoche)lpParam), MiPermisoAvance, MiConfirmacionAvance) == -1)
        fprintf(stderr, "Error pP2_desaparcar %d\n", pP2_getNumero((HCoche)lpParam));
    return 0;
}

int llegada_peorAjuste(HCoche hc) {
    int lon = pP2_getLongitud(hc), alg = PEOR_AJUSTE, pos = 0, h, i, pP = -1, pH = -1;
    EsperaSegura(hMutexAcera[alg], "Mutex Peor");
    while (pos <= LONGITUD_ACERA - lon) {
        if (!aceras[alg][pos]) {
            h = 0; while (pos + h < LONGITUD_ACERA && !aceras[alg][pos + h]) h++;
            if (h >= lon && h > pH) { pH = h; pP = pos; }
            pos += h;
        }
        else pos++;
    }
    if (pP != -1) {
        for (i = 0; i < lon; i++) aceras[alg][pP + i] = 1;
        HANDLE hH = CreateThread(NULL, 0, HiloAparcar, (LPVOID)hc, 0, NULL);
        if (hH) CloseHandle(hH);
        else {
            for (i = 0; i < lon; i++) aceras[alg][pP + i] = 0;
            pP = -1;
        }
    }
    LiberacionSegura(hMutexAcera[alg], "Release Peor");
    return pP;
}

int llegada_primerAjuste(HCoche hc) {
    int lon = pP2_getLongitud(hc), alg = PRIMER_AJUSTE, pos = 0, h, i, res = -1;
    EsperaSegura(hMutexAcera[alg], "Mutex Primer");
    while (pos <= LONGITUD_ACERA - lon) {
        if (!aceras[alg][pos]) {
            h = 0; while (pos + h < LONGITUD_ACERA && !aceras[alg][pos + h]) h++;
            if (h >= lon) {
                res = pos;
                for (i = 0; i < lon; i++) aceras[alg][pos + i] = 1;
                HANDLE hH = CreateThread(NULL, 0, HiloAparcar, (LPVOID)hc, 0, NULL);
                if (hH) CloseHandle(hH);
                else {
                    for (i = 0; i < lon; i++) aceras[alg][res + i] = 0;
                    res = -1;
                }
                break;
            }
            pos += h;
        }
        else pos++;
    }
    LiberacionSegura(hMutexAcera[alg], "Release Primer");
    return res;
}

int llegada_siguienteAjuste(HCoche hc) {
    int lon = pP2_getLongitud(hc), alg = SIGUIENTE_AJUSTE, num = pP2_getNumero(hc);
    int inicio, pos, h, i, res = -1;

    EsperaSegura(hMutexAcera[alg], "Mutex Siguiente");
    inicio = ult_pos_siguiente[alg];

    if (!aceras[alg][inicio]) {
        while (inicio > 0 && !aceras[alg][inicio - 1]) inicio--;
    }

    pos = inicio;
    for (int v = 0; v < LONGITUD_ACERA; v++) {
        if (pos <= LONGITUD_ACERA - lon && !aceras[alg][pos]) {
            h = 0;
            while (pos + h < LONGITUD_ACERA && !aceras[alg][pos + h]) h++;
            if (h >= lon) {
                res = pos;
                for (i = 0; i < lon; i++) aceras[alg][pos + i] = 1;
                ult_pos_siguiente[alg] = (pos + lon) % LONGITUD_ACERA;
                HANDLE hH = CreateThread(NULL, 0, HiloAparcar, (LPVOID)hc, 0, NULL);
                if (hH) CloseHandle(hH);
                else {
                    for (i = 0; i < lon; i++) aceras[alg][res + i] = 0;
                    res = -1;
                }
                break;
            }
            pos += h;
        }
        else pos = (pos + 1) % LONGITUD_ACERA;
    }
    LiberacionSegura(hMutexAcera[alg], "Release Siguiente");
    return res;
}

int llegada_mejorAjuste(HCoche hc) {
    int lon = pP2_getLongitud(hc), alg = MEJOR_AJUSTE, pos = 0, h, i, mP = -1, mH = 81;
    EsperaSegura(hMutexAcera[alg], "Mutex Mejor");
    while (pos <= LONGITUD_ACERA - lon) {
        if (!aceras[alg][pos]) {
            h = 0; while (pos + h < LONGITUD_ACERA && !aceras[alg][pos + h]) h++;
            if (h >= lon && h < mH) { mH = h; mP = pos; }
            pos += h;
        }
        else pos++;
    }
    if (mP != -1) {
        for (i = 0; i < lon; i++) aceras[alg][mP + i] = 1;
        HANDLE hH = CreateThread(NULL, 0, HiloAparcar, (LPVOID)hc, 0, NULL);
        if (hH) CloseHandle(hH);
        else {
            for (i = 0; i < lon; i++) aceras[alg][mP + i] = 0;
            mP = -1;
        }
    }
    LiberacionSegura(hMutexAcera[alg], "Release Mejor");
    return mP;
}

int logica_salida(HCoche hc) {
    HANDLE hH = CreateThread(NULL, 0, HiloDesaparcar, (LPVOID)hc, 0, NULL);
    if (hH) { CloseHandle(hH); return 0; }
    return -1;
}

int salida_primerAjuste(HCoche hc) { return logica_salida(hc); }
int salida_siguienteAjuste(HCoche hc) { return logica_salida(hc); }
int salida_mejorAjuste(HCoche hc) { return logica_salida(hc); }
int salida_peorAjuste(HCoche hc) { return logica_salida(hc); }

int main(int argc, char* argv[]) {
    HINSTANCE hDLL; TIPO_FUNCION_LLEGADA fl[4]; TIPO_FUNCION_SALIDA fs[4];
    if (argc < 2) {
        printf("Uso: ./parking2 [delay] [D]\n");
        return 1;
    }
    if (!(hDLL = LoadLibrary("parking2.dll"))) { 
        fprintf(stderr, "Error DLL\n"); 
        return 1; }

#define OBT_DIR(v, n, t) v = (t)GetProcAddress(hDLL, n); if(!v) { fprintf(stderr, "Error func %s\n", n); return 1; }
    OBT_DIR(pP2_inicio, "PARKING2_inicio", T_P2_inicio);
    OBT_DIR(pP2_fin, "PARKING2_fin", T_P2_fin);
    OBT_DIR(pP2_aparcar, "PARKING2_aparcar", T_P2_aparcar);
    OBT_DIR(pP2_desaparcar, "PARKING2_desaparcar", T_P2_desaparcar);
    OBT_DIR(pP2_getNumero, "PARKING2_getNUmero", T_P2_getNumero);
    OBT_DIR(pP2_getLongitud, "PARKING2_getLongitud", T_P2_getLongitud);
    OBT_DIR(pP2_getPosAcera, "PARKING2_getPosiciOnEnAcera", T_P2_getPosAcera);
    OBT_DIR(pP2_getAlgoritmo, "PARKING2_getAlgoritmo", T_P2_getAlgoritmo);
    OBT_DIR(pP2_getDatos, "PARKING2_getDatos", T_P2_getDatos);
    OBT_DIR(pP2_getX2, "PARKING2_getX2", T_P2_getX2);
    OBT_DIR(pP2_getY2, "PARKING2_getY2", T_P2_getY2);
    OBT_DIR(pP2_getX, "PARKING2_getX", T_P2_getX);
    OBT_DIR(pP2_getY, "PARKING2_getY", T_P2_getY);

    for (int i = 0; i < 4; i++) {
        if (!(hMutexAcera[i] = CreateMutex(NULL, FALSE, NULL))) return 1;
        if (!(hEventoTurno[i] = CreateEvent(NULL, FALSE, FALSE, NULL))) return 1;
        if (!(hEventoCarril[i] = CreateEvent(NULL, TRUE, FALSE, NULL))) return 1;
    }

    fl[0] = llegada_primerAjuste; fl[1] = llegada_siguienteAjuste; fl[2] = llegada_mejorAjuste; fl[3] = llegada_peorAjuste;
    fs[0] = salida_primerAjuste; fs[1] = salida_siguienteAjuste; fs[2] = salida_mejorAjuste; fs[3] = salida_peorAjuste;

    if (pP2_inicio(fl, fs, atol(argv[1]), (argc > 2 && strcmp(argv[2], "D") == 0)) == -1) return 1;
    Sleep(30000);
    if (pP2_fin() == -1) fprintf(stderr, "Error fin sim\n");

    for (int i = 0; i < 4; i++) { 
        CloseHandle(hMutexAcera[i]); 
        CloseHandle(hEventoTurno[i]); 
        CloseHandle(hEventoCarril[i]); }
    FreeLibrary(hDLL);
    return 0;
}