#include <pthread.h>

typedef struct no No;

typedef struct cabecaFila Fila;

typedef struct escalonador Escalonador;

Escalonador* escalonador;

int novoProcesso;

int escalonamentoTerminou;

int pidInterpretador;

Escalonador * escalonadorCria (void);

void escalonamento (Escalonador * escalonador);

void escreveStatus (Escalonador * escalonador);

void gerenciaFilaNovos (Escalonador * escalonador);


