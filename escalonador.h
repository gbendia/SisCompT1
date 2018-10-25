#include <pthread.h>

typedef struct no No;

typedef struct cabecaFila Fila;

typedef struct escalonador Escalonador;

Escalonador* escalonador;

pthread_t threadsIDs[7];

int threadsPausadas[7];

Escalonador * escalonadorCria (void);

void escalonamento (Escalonador * escalonador);

void escreveStatus (Escalonador * escalonador);

void gerenciaFilaNovos (Escalonador * escalonador);


