#include "escalonador.h"
#include "interpretador.h"
#include <pthread.h>
#include <stdio.h>

#define N 4

Escalonador* escalonador;
int flag;

void thread_main (void * t)
{
	int caso = (int) t;
	switch (caso)
	{
		case 1:
			interpretador ();
			break;
		case 2:
			gerenciaFilaNovos (escalonador);
			break;
		case 3:
			flag = 1;
			escalonamento (escalonador);
			break;
		case 4:
			while(flag != 1);
			escreveStatus (escalonador);
			break;
		default:
			printf("Erro na main das threads\n");
	}
}

int main(void)
{
	int i;
	pthread_t threads[N];

	flag = 0;

	escalonador = escalonadorCria ();

	for(i = 0 ; i < N ; i++)
	{
		pthread_create(&threads[i], NULL, thread_main, (void*) i+1);
	}

	for(i = 0; i < N; i++) {
		pthread_join(threads[i],NULL); // Espera todas as threads terminarem
	}

	return 0;
}
