#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>

#include "interpretador.h"

void interpretador (void) //joga na memoria compartilhada os nomes e as prioridades dos programas
{
	int idMemPr, idMemNome, i=0;
	char * nome;
	char comando [101], aux[81];
	int *prioridade;
	int numPr = 3956543;
	int numNome = 4354567;
	FILE * arq = fopen("exec.txt", "r");
	if (arq == NULL)
	{
		printf("Erro na abertura do arquivo de entrada\n");
		exit(1);
	}
	
	idMemNome = shmget(numNome, 81*sizeof(char), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	idMemPr = shmget(numPr, sizeof(int*), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	nome = (char*) shmat(idMemNome, 0, 0);
	prioridade = (int*) shmat(idMemPr, 0, 0);

	while(fscanf(arq," %[^\n]", comando) == 1)
	{
		 
		for(i=0; comando[i+5] != ' '; i++) //i+5 para tirar o "exec "
		{
			aux[i] = comando[i+5];
		}
		aux[i] = '\0';


		strcpy(nome, aux);
		(*prioridade) = (int) ( comando [strlen(comando)-1] - '0');

//		printf("Mandei sinal de novo processo\n");
		kill(getppid(), SIGUSR1); //avisa para o pai que há um processo novo na memória compartilhada

		kill(getpid(), SIGSTOP); // para até o pai mandar continuar --> indica que leu o dado na memória

//		printf("Interpretador - Continuei\n");		
		sleep(1);

//		printf("Interpretador - Acordei\n");	

	}

	printf("Interpretador terminou\n");
	// libera a memória compartilhada do processo
  	shmdt (nome);
	shmdt (prioridade);

  	// libera a memória compartilhada
  	shmctl (idMemNome, IPC_RMID, 0);
	shmctl (idMemPr, IPC_RMID, 0);

	fclose(arq);
}
