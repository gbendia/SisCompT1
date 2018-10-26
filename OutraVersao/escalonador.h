#include <pthread.h>
#define N 7

typedef struct no No;

typedef struct cabecaFila Fila;

typedef struct escalonador Escalonador;

Escalonador* escalonador;

int novoProcesso;

int escalonamentoTerminou;

int pidInterpretador;

int executando[N];

int pids[N];

Escalonador * escalonadorCria (void);

void escalonamento (Escalonador * escalonador);

void escreveStatus (Escalonador * escalonador);

void gerenciaFilaNovos (Escalonador * escalonador);


