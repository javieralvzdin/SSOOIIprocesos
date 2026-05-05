#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <errno.h>
#include "parking.h"

#define LONGITUD_ACERA 80
#define NUM_ALGORITMOS 4
#define DURACION_SIM 30
#define MAX_HIJOS 200

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

typedef enum { POL_NINGUNA, POL_PA, POL_PD } TipoPolitica;

typedef struct {
    int ocupado[LONGITUD_ACERA];
    int carril[LONGITUD_ACERA];
} EstadoAcera;

typedef struct {
    EstadoAcera aceras[NUM_ALGORITMOS];
    int ultimaPosSiguiente[NUM_ALGORITMOS];
    int turno_aparcar[NUM_ALGORITMOS];
} MiZonaShared;

typedef struct {
    long mtype;
    struct PARKING_mensajeBiblioteca msg;
} MsgChofer;

static int g_semId = -1, g_buzId = -1, g_shmId = -1;
static void *g_shm = NULL;
static TipoPolitica g_politica = POL_NINGUNA;
static MiZonaShared *g_misDatos = NULL;
static pid_t g_hijos[MAX_HIJOS];
static int g_nHijos = 0;
static int g_nSemLib = 0, g_nChoferes = 0, g_retardo = 0, g_debug = 0;

#define IDX_MUTEX_ACERA(a) (g_nSemLib + (a))
#define IDX_SEM_TURNO (g_nSemLib + 4)

static void limpiarRecursos(void) {
    if (g_shm && g_shm != (void*)-1) shmdt(g_shm);
    if (g_semId != -1) semctl(g_semId, 0, IPC_RMID);
    if (g_buzId != -1) msgctl(g_buzId, IPC_RMID, NULL);
    if (g_shmId != -1) shmctl(g_shmId, IPC_RMID, NULL);
}

void manejadoraSIGINT(int sig) {
    int i;
    signal(SIGINT, SIG_IGN);
    for (i = 0; i < g_nHijos; i++) if (g_hijos[i] > 0) kill(g_hijos[i], SIGKILL);
    while (wait(NULL) > 0);
    limpiarRecursos();
    _exit(0);
}

static void sem_op(int idx, int val) {
    struct sembuf op;
    op.sem_num = (unsigned short)idx;
    op.sem_op = (short)val;
    op.sem_flg = 0;
    while (semop(g_semId, &op, 1) == -1) {
        if (errno != EINTR) { manejadoraSIGINT(0); } 
    }
}
#define W(i) sem_op(i, -1)
#define S(i) sem_op(i, 1)

void manejadora_alarma(int sig) { PARKING_fin(1); _exit(0); }

int llegada_primerAjuste(HCoche hc) {
    int lon = PARKING_getLongitud(hc), alg = PRIMER_AJUSTE, pos = 0, h, i, res = -1;
    W(IDX_MUTEX_ACERA(alg));
    while (pos <= LONGITUD_ACERA - lon) {
        if (!g_misDatos->aceras[alg].ocupado[pos]) {
            h = 0; while (pos + h < LONGITUD_ACERA && !g_misDatos->aceras[alg].ocupado[pos + h]) h++;
            if (h >= lon) { res = pos; for (i = 0; i < lon; i++) g_misDatos->aceras[alg].ocupado[pos + i] = 1; break; }
            pos += h;
        } else pos++;
    }
    S(IDX_MUTEX_ACERA(alg)); return res;
}

int llegada_siguienteAjuste(HCoche hc) {
    int lon = PARKING_getLongitud(hc), alg = SIGUIENTE_AJUSTE;
    int inicio, pos, vueltas, h, i, res = -1;

    W(IDX_MUTEX_ACERA(alg));
    inicio = g_misDatos->ultimaPosSiguiente[alg];

    while (inicio > 0 && !g_misDatos->aceras[alg].ocupado[inicio - 1]) {
        inicio--;
    }

    pos = inicio;
    for (vueltas = 0; vueltas < LONGITUD_ACERA; vueltas++) {
        if (pos <= LONGITUD_ACERA - lon && !g_misDatos->aceras[alg].ocupado[pos]) {
            h = 0; while (pos + h < LONGITUD_ACERA && !g_misDatos->aceras[alg].ocupado[pos + h]) h++;
            if (h >= lon) {
                res = pos;
                for (i = 0; i < lon; i++) g_misDatos->aceras[alg].ocupado[pos + i] = 1;
                g_misDatos->ultimaPosSiguiente[alg] = (pos + lon) % LONGITUD_ACERA;
                break;
            }
        }
        pos = (pos + 1) % LONGITUD_ACERA;
    }
    S(IDX_MUTEX_ACERA(alg)); return res;
}

int llegada_mejorAjuste(HCoche hc) {
    int lon = PARKING_getLongitud(hc), alg = MEJOR_AJUSTE, pos = 0, h, i, mP = -1, mH = LONGITUD_ACERA + 1;
    W(IDX_MUTEX_ACERA(alg));
    while (pos <= LONGITUD_ACERA - lon) {
        if (!g_misDatos->aceras[alg].ocupado[pos]) {
            h = 0; while (pos + h < LONGITUD_ACERA && !g_misDatos->aceras[alg].ocupado[pos + h]) h++;
            if (h >= lon && h < mH) { mH = h; mP = pos; }
            pos += h;
        } else pos++;
    }
    if (mP != -1) for (i = 0; i < lon; i++) g_misDatos->aceras[alg].ocupado[mP + i] = 1;
    S(IDX_MUTEX_ACERA(alg)); return mP;
}

int llegada_peorAjuste(HCoche hc) {
    int lon = PARKING_getLongitud(hc), alg = PEOR_AJUSTE, pos = 0, h, i, pP = -1, pH = -1;
    W(IDX_MUTEX_ACERA(alg));
    while (pos <= LONGITUD_ACERA - lon) {
        if (!g_misDatos->aceras[alg].ocupado[pos]) {
            h = 0; while (pos + h < LONGITUD_ACERA && !g_misDatos->aceras[alg].ocupado[pos + h]) h++;
            if (h >= lon && h > pH) { pH = h; pP = pos; }
            pos += h;
        } else pos++;
    }
    if (pP != -1) for (i = 0; i < lon; i++) g_misDatos->aceras[alg].ocupado[pP + i] = 1;
    S(IDX_MUTEX_ACERA(alg)); return pP;
}

static void aparcarCommit(HCoche hc) {
    int alg = PARKING_getAlgoritmo(hc), i;
    union semun arg;
    W(IDX_MUTEX_ACERA(alg));
    g_misDatos->turno_aparcar[alg]++;
    S(IDX_MUTEX_ACERA(alg));

    arg.val = 0;
    semctl(g_semId, IDX_SEM_TURNO, SETVAL, arg);
    for (i = 0; i < g_nChoferes; i++) S(IDX_SEM_TURNO);
}

static void permisoAvance(HCoche hc) {
    int alg = PARKING_getAlgoritmo(hc), x2 = PARKING_getX2(hc), y2 = PARKING_getY2(hc);
    int lon = PARKING_getLongitud(hc), num = PARKING_getNUmero(hc), i, seguro;
    while (1) {
        W(IDX_MUTEX_ACERA(alg));
        seguro = 1;
        if (y2 == 2) {
            for (i = x2; i < x2 + lon && i < LONGITUD_ACERA; i++)
                if (i >= 0 && g_misDatos->aceras[alg].carril[i] != 0 && g_misDatos->aceras[alg].carril[i] != num)
                    seguro = 0;
        }
        if (seguro) {
            if (y2 == 2) for (i = x2; i < x2 + lon && i < LONGITUD_ACERA; i++) if (i >= 0) g_misDatos->aceras[alg].carril[i] = num;
            S(IDX_MUTEX_ACERA(alg)); break;
        }
        S(IDX_MUTEX_ACERA(alg)); usleep(10000);
    }
}

static void permisoAvanceCommit(HCoche hc) {
    int alg = PARKING_getAlgoritmo(hc), x = PARKING_getX(hc), y = PARKING_getY(hc), lon = PARKING_getLongitud(hc), num = PARKING_getNUmero(hc), i, pos;
    W(IDX_MUTEX_ACERA(alg));
    for (i = 0; i < LONGITUD_ACERA; i++) if (g_misDatos->aceras[alg].carril[i] == num) g_misDatos->aceras[alg].carril[i] = 0;
    if (y == 2) for (i = x; i < x + lon && i < LONGITUD_ACERA; i++) if (i >= 0) g_misDatos->aceras[alg].carril[i] = num;
    if (y == 2 && PARKING_getY2(hc) < 2) {
        pos = (int)(long)PARKING_getDatos(hc);
        if (pos >= 0) for (i = 0; i < lon; i++) g_misDatos->aceras[alg].ocupado[pos + i] = 0;
    }
    S(IDX_MUTEX_ACERA(alg));
}

static void procesoChofer(void) {
    MsgChofer m;
    while (1) {
        if (msgrcv(g_buzId, &m, sizeof(m.msg), -2, 0) == -1) {
            if (errno == EINTR) continue; _exit(0);
        }
        if (m.msg.subtipo == PARKING_MSGSUB_APARCAR) {
            int alg = PARKING_getAlgoritmo(m.msg.hCoche), num = PARKING_getNUmero(m.msg.hCoche);
            while (1) {
                W(IDX_MUTEX_ACERA(alg));
                if (g_misDatos->turno_aparcar[alg] == num) { S(IDX_MUTEX_ACERA(alg)); break; }
                S(IDX_MUTEX_ACERA(alg)); W(IDX_SEM_TURNO);
            }
            PARKING_aparcar(m.msg.hCoche, (void*)(long)PARKING_getPosiciOnEnAcera(m.msg.hCoche), aparcarCommit, permisoAvance, permisoAvanceCommit);
        } else {
            PARKING_desaparcar(m.msg.hCoche, PARKING_getDatos(m.msg.hCoche), permisoAvance, permisoAvanceCommit);
        }
    }
}

static void procesoGestor(void) {
    struct PARKING_mensajeBiblioteca mL; MsgChofer mC;
    while (1) {
        if (msgrcv(g_buzId, &mL, sizeof(mL) - sizeof(long), PARKING_MSG, 0) == -1) {
            if (errno == EINTR) continue; _exit(0);
        }
        mC.msg = mL;
        if (g_politica == POL_PA) mC.mtype = (mL.subtipo == PARKING_MSGSUB_APARCAR) ? 1 : 2;
        else if (g_politica == POL_PD) mC.mtype = (mL.subtipo == PARKING_MSGSUB_DESAPARCAR) ? 1 : 2;
        else mC.mtype = 1;
        msgsnd(g_buzId, &mC, sizeof(mC.msg), 0);
    }
}

int main(int argc, char *argv[]) {
    int i, nBytesLib; pid_t pid; TIPO_FUNCION_LLEGADA fl[4]; union semun arg;
    if (argc < 3) { fprintf(stderr, "Uso: %s <rapidez> <choferes> [PA|PD]\n", argv[0]); return 1; }
    g_retardo = atoi(argv[1]); g_nChoferes = atoi(argv[2]);
    for (i = 3; i < argc; i++) {
        if (strcmp(argv[i], "PA") == 0) g_politica = POL_PA;
        else if (strcmp(argv[i], "PD") == 0) g_politica = POL_PD;
        else if (strcmp(argv[i], "D") == 0) g_debug = 1;
    }
    signal(SIGINT, manejadoraSIGINT);
    g_nSemLib = PARKING_getNSemAforos();
    nBytesLib = (PARKING_getTamaNoMemoriaCompartida() + 3) & ~3;
    g_semId = semget(IPC_PRIVATE, g_nSemLib + 5, IPC_CREAT | 0600);
    g_buzId = msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    g_shmId = shmget(IPC_PRIVATE, nBytesLib + sizeof(MiZonaShared), IPC_CREAT | 0600);
    g_shm = shmat(g_shmId, NULL, 0);
    g_misDatos = (MiZonaShared*)((char*)g_shm + nBytesLib);
    memset(g_misDatos, 0, sizeof(MiZonaShared));
    for (i = 0; i < 4; i++) g_misDatos->turno_aparcar[i] = 1;
    for (i = 0; i < 4; i++) { arg.val = 1; semctl(g_semId, g_nSemLib + i, SETVAL, arg); }
    arg.val = 0; semctl(g_semId, g_nSemLib + 4, SETVAL, arg);
    fl[0] = llegada_primerAjuste; fl[1] = llegada_siguienteAjuste; fl[2] = llegada_mejorAjuste; fl[3] = llegada_peorAjuste;
    PARKING_inicio(g_retardo, fl, g_semId, g_buzId, g_shmId, g_debug);
    if ((pid = fork()) == 0) { signal(SIGALRM, manejadora_alarma); alarm(DURACION_SIM); while (1) pause(); _exit(0); }
    if (pid > 0) g_hijos[g_nHijos++] = pid;
    if ((pid = fork()) == 0) { procesoGestor(); _exit(0); }
    if (pid > 0) g_hijos[g_nHijos++] = pid;
    for (i = 0; i < g_nChoferes; i++) {
        pid = fork();
        if (pid == 0) { procesoChofer(); _exit(0); }
        if (pid > 0) g_hijos[g_nHijos++] = pid;
    }
    PARKING_simulaciOn();
    printf("\nSimulacion terminada. Pulsa Enter para salir...");
    fflush(stdout); while (fgetc(stdin) != '\n');
    manejadoraSIGINT(0);
    return 0;
}