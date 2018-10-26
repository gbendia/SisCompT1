#include "escalonador.h"
#include "interpretador.h"
#include "semaforo.h"
#include <pthread.h>
#include <stdio.h>

#define M 4


void thread_main (void * t)
{
	escreveStatus (escalonador);
}

int main(void)
{
	pthread_t threads[1];

	int statloc;

	novoProcesso = 0;

	escalonamentoTerminou = 0;

	escalonador = escalonadorCria ();

	pidInterpretador = fork();
	
	if(pidInterpretador == 0)
	{
		interpretador ();
		exit(0);
	}

	pthread_create(&threads[0], NULL, thread_main, NULL);

	escalonamento(escalonador);

	waitpid(-1, &statloc,0);

	pthread_join(threads[0],NULL); // Espera todas as threads terminarem

	printf("Fim escalonamento \n");


	return 0;
}
