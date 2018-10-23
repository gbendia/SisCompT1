#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include "escalonador.h"
#define TEMPOMAX 10

struct no
{
	struct no * prox;
	char nomeProg[81];
};

struct cabecaFila
{
	struct no *primeiro;
	struct no *ultimo;
	int prioridade;
	int qntElementos;
};

struct escalonador
{
	Fila * prioridade1;
	Fila * prioridade2;
	Fila * prioridade3;
	Fila * prioridade4;
	Fila * prioridade5;
	Fila * prioridade6;
	Fila * prioridade7;
	Fila * terminados;
	Fila * emEspera;
	char emExecucao[81];

	clock_t inicio;
};

Fila * filaCria (int prioridade)
{
	Fila * nova = (Fila*) malloc (sizeof (Fila));
	if (nova == NULL)
	{
		printf("Erro na criacao da fila de prioridade %d\n", prioridade);
		exit(1);
	}

	nova->primeiro = NULL;
	nova->ultimo = NULL;
	nova->prioridade = prioridade;
	nova->qntElementos = 0;
	
	return nova;
}

int filaVazia (Fila * fila)
{
	if (fila->qntElementos == 0)
		return 1;
	else
		return 0;
}

void insereNoPrioridade (int prioridade, char * nomeProg, Escalonador * escalona)
{
	No * novo = (No*) malloc (sizeof(No));
	Fila * aux;
	if (novo == NULL)
	{
		printf("Erro na criacao do no do programa %s\n", nomeProg);
		exit(1);
	}

	strcpy(novo->nomeProg, nomeProg);
	
	novo->prox = NULL;

	switch (prioridade)
	{
		case 1:
			aux = escalona->prioridade1;
			break;
		case 2:
			aux = escalona->prioridade2;
			break;
		case 3:
			aux = escalona->prioridade3;
			break;
		case 4:
			aux = escalona->prioridade4;
			break;
		case 5:
			aux = escalona->prioridade5;
			break;
		case 6:
			aux = escalona->prioridade6;
			break;
		case 7:
			aux = escalona->prioridade7;
			break;
		case -1:
			aux = escalona->terminados;
			break;
		default:
			aux = NULL;
	}


	if (filaVazia(aux))
	{
		aux->primeiro = novo;
	}
	else
	{
		aux->ultimo->prox = novo;
	}
	aux->ultimo = novo;
	aux->qntElementos ++;
	
	return;
}

char * retiraNo (Fila * fila)
{
	char * str = NULL;
	No * aux;

	if (!filaVazia(fila))
	{
		str =  (char*) malloc ( (strlen(fila->primeiro->nomeProg) + 1)*sizeof(char));
		if (str == NULL)
		{	
			printf("Erro na alocacao da string retirada da fila\n");
			exit(1);
		}
	
		strcpy(str, fila->primeiro->nomeProg);
		aux = fila->primeiro;
		if (fila->qntElementos == 1) // só tinha um elemento na fila
		{
			fila->ultimo = NULL;
		}
		fila->primeiro = fila->primeiro->prox;
		free(aux);
		fila->qntElementos --;
	}
	return str;
}

Escalonador * escalonadorCria (void)
{
	Escalonador * escalona = (Escalonador*) malloc (sizeof(Escalonador));
	if (escalona == NULL)
	{
		printf("Erro na criacao do escalonador\n");
		exit(1);
	}

	escalona->prioridade1 = filaCria (1);
	escalona->prioridade2 = filaCria (2);
	escalona->prioridade3 = filaCria (3);
	escalona->prioridade4 = filaCria (4);
	escalona->prioridade5 = filaCria (5);
	escalona->prioridade6 = filaCria (6);
	escalona->prioridade7 = filaCria (7);
	escalona->terminados = filaCria (-1);
	escalona->emEspera = filaCria (-2);
	strcpy(escalona->emExecucao, "nenhum");
	
	escalona->inicio = clock();

	return escalona;
}

void usr1handler(int signo)
{
	printf("Sinal usr1 recebido\n");
	kill(getpid(), SIGCONT);
}


void gerenciaFilaEspecifica (Fila * filaPrioridade, char * emExecucao, Escalonador* escalonador)
{
	int pid;
	int pidGerenciador = getpid();
	char *nomeProg;
	printf("Entrei na gerencia\n");
	kill(getpid(), SIGSTOP);
	printf("Continuei\n");
	printf("Informacoes fila prioridade %d, numELem: %d\n", filaPrioridade->prioridade, filaPrioridade->qntElementos);
	while(1)
	{
		while( !filaVazia(filaPrioridade) )
		{
			printf("Filho com prioridade %d executando\n", filaPrioridade->prioridade);
			nomeProg = retiraNo (filaPrioridade);
			printf("Nome do programa a ser executado: %s\n", nomeProg);
			pid = fork();
			if(pid==0) // filho executa o programa
			{
				printf("Está no processo filho\n");
				strcpy(emExecucao,nomeProg);
				execv(nomeProg, NULL);
				exit(0);
			}
			else // pai dorme enquanto escalonador não avisa que ultrapassou o tempo
			{
				printf("Está no processo pai\n");
				sleep(3);
				printf("Pai acordou\n");

				insereNoPrioridade (-1, nomeProg, escalonador); // inserir na fila de terminados
				printf("Inseriu na fila de terminados\n");
				//Quando volta, mata o filho
				kill(pid, SIGKILL);
				printf("Matou o processo executando\n");
			}
		}
		printf("Pai parou\n");
		kill(getpid(), SIGSTOP);
	}
}


void escalonamento (Escalonador * escalonador)
{
	int flagTempos[] = {0,0,0,0,0,0,0};
	int prioridadeAtiva;
	double dt, maxDt;
	clock_t primeirosTempos[7];
	int pid[7]; //guarda os pids dos filhos
	No * aux;
	int i;

	sleep(3);
	printf("Comeco escalonamento\n");

	for(i=0;i<7;i++)
	{
		pid[i] = fork(); 
		if(pid[i] == 0) //cada processo filho
		{
			printf("Fork %d\n", i);
			switch (i)
			{
				case 0:
					gerenciaFilaEspecifica (escalonador->prioridade1, escalonador->emExecucao, escalonador);
					break;
				case 1:
					gerenciaFilaEspecifica (escalonador->prioridade2, escalonador->emExecucao, escalonador);
					break;
				case 2:
					gerenciaFilaEspecifica (escalonador->prioridade3, escalonador->emExecucao, escalonador);
					break;
				case 3:
					gerenciaFilaEspecifica (escalonador->prioridade4, escalonador->emExecucao, escalonador);
					break;
				case 4:
					gerenciaFilaEspecifica (escalonador->prioridade5, escalonador->emExecucao, escalonador);
					break;
				case 5:
					gerenciaFilaEspecifica (escalonador->prioridade6, escalonador->emExecucao, escalonador);
					break;
				case 6:
					gerenciaFilaEspecifica (escalonador->prioridade7, escalonador->emExecucao, escalonador);
					break;
				default:
					break;
			}
			exit(0);
		}
	}
	//processo pai: gerencia qual filho está ativo em cada momento
	while(1)
	{
		if( !filaVazia (escalonador->prioridade1) ) // filho 0 executa
		{
			printf("Fila de prioridade 1: num elem: %d\n", escalonador->prioridade1->qntElementos);
			for(i=1;i<7;i++)
				kill(pid[i], SIGSTOP);

			kill(pid[0], SIGCONT);
		}
		else if( !filaVazia (escalonador->prioridade2) ) // filho 1 executa
		{
			printf("Fila de prioridade 2: num elem: %d\n", escalonador->prioridade2->qntElementos);
			for(i=2;i<7;i++)
				kill(pid[i], SIGSTOP);

			kill(pid[1], SIGCONT);
		}
		else if( !filaVazia (escalonador->prioridade3) ) // filho 2 executa
		{
			printf("Fila de prioridade 3: num elem: %d\n", escalonador->prioridade3->qntElementos);
			for(i=3;i<7;i++)
				kill(pid[i], SIGSTOP);

			kill(pid[2], SIGCONT);
		}
		else if( !filaVazia (escalonador->prioridade4) ) // filho 3 executa
		{
			printf("Fila de prioridade 4: num elem: %d\n", escalonador->prioridade4->qntElementos);
			for(i=4;i<7;i++)
				kill(pid[i], SIGSTOP);

			kill(pid[3], SIGCONT);
		}
		else if( !filaVazia (escalonador->prioridade5) ) // filho 4 executa
		{
			printf("Fila de prioridade 5: num elem: %d\n", escalonador->prioridade5->qntElementos);
			for(i=5;i<7;i++)
				kill(pid[i], SIGSTOP);

			kill(pid[4], SIGCONT);
		}
		else if( !filaVazia (escalonador->prioridade6) ) // filho 5 executa //ROUND_ROBIN
		{
			printf("Fila de prioridade 6: num elem: %d\n", escalonador->prioridade6->qntElementos);
			for(i=6;i<7;i++)
				kill(pid[i], SIGSTOP);

			kill(pid[5], SIGCONT);
			
		}
		else if( !filaVazia (escalonador->prioridade7) ) // filho 6 executa //ROUND_ROBIN
		{
			printf("Fila de prioridade 7: num elem: %d\n", escalonador->prioridade7->qntElementos);
			kill(pid[6], SIGCONT);
		}

		// Maneja os tempos de execucao
		for(i=0;i<7;i++)
		{
			if(i != prioridadeAtiva)
			{
				flagTempos[i] = 0;	
			}
		}
		if(flagTempos[prioridadeAtiva] == 0)
		{
			primeirosTempos[prioridadeAtiva] = clock();
			flagTempos[prioridadeAtiva] = 1;
		}
		else
		{
			dt = (double)((clock() - primeirosTempos[prioridadeAtiva])/CLOCKS_PER_SEC);
			if(dt > maxDt)
			{
				flagTempos[prioridadeAtiva] = 0;
			//	kill(pid[prioridadeAtiva], SIGUSR1); //indica para o filho que a fatia de tempo acabou
			}
		}
	}
}

void gerenciaFilaNovos (Escalonador * escalonador)
{
	int numPr = 2310;
	int numNome = 1108;
	char * nome;
	int * prioridade;
	int idMemNome, idMemPr;

	sleep(1);

	idMemNome = shmget(numNome, 81*sizeof(char), IPC_EXCL | S_IRUSR | S_IWUSR);
	nome = (char*) shmat(idMemNome, NULL, 0);	

	while(nome != NULL && strcmp(nome, "FIMDADOSARQUIVO") != 0)
	{

		idMemPr = shmget(numPr, sizeof(int*), IPC_EXCL | S_IRUSR | S_IWUSR);
		prioridade = (int*) shmat(idMemPr, 0, 0);
		
		insereNoPrioridade (*prioridade, nome , escalonador);

		// libera a memória compartilhada do processo
  	shmdt (nome);
		shmdt (prioridade);

  	// libera a memória compartilhada
  	shmctl (idMemNome, IPC_RMID, 0);
		shmctl (idMemPr, IPC_RMID, 0);

		numPr ++;
		numNome ++;

		sleep(1);

		idMemNome = shmget(numNome, 81*sizeof(char), IPC_EXCL | S_IRUSR | S_IWUSR);
		nome = (char*) shmat(idMemNome, 0, 0);	
	}

	// libera a memória compartilhada do processo
 	shmdt (nome);
 	// libera a memória compartilhada
 	shmctl (idMemNome, IPC_RMID, 0);
}

void escreveFilas (FILE* arq, Fila * fila)
{
	No * aux;
	if(fila == NULL)
		return;
	fprintf(arq, "\tNumero de elementos: %d\n",fila->qntElementos);
	if (fila->qntElementos == 0)
	{
		fprintf(arq, "\tNao ha elementos nessa fila\n");
	}
	else
	{
		for(aux = fila->primeiro; aux != NULL; aux = aux->prox)
		{
			fprintf(arq, "\tPrograma %s\n", aux->nomeProg);
		}
	}
}

void escreveStatus (Escalonador * escalonador)
{
	FILE* arq; 

	int i = 0;
	//clock_t dt;

	arq = fopen ("saida.txt","w");
	if (arq == NULL)
	{
		printf("Erro na abertura do arquivo de saida\n");
		exit(1);
	}
	fprintf(arq, "Inicio do escalonamento\n");

	while(i<10)
	{
 		i++;
		if (escalonador != NULL)
		{
			//dt = (clock() - escalonador->inicio)/CLOCKS_PER_SEC;
			//fprintf(arq, "******************** Em t = %.2f ********************\n", (double) dt);
			fprintf(arq, "******************** Em t = %d s********************\n", i);
			fprintf(arq, "***********Status das filas de prontos***********\n");
			fprintf(arq, "\t**Prioridade 1**\n\n");
			escreveFilas(arq, escalonador->prioridade1);
			fprintf(arq, "\n\t**Prioridade 2**\n\n");
			escreveFilas(arq, escalonador->prioridade2);
			fprintf(arq, "\n\t**Prioridade 3**\n\n");
			escreveFilas(arq, escalonador->prioridade3);
			fprintf(arq, "\t**Prioridade 4**\n\n");
			escreveFilas(arq, escalonador->prioridade4);
			fprintf(arq, "\n\t**Prioridade 5**\n\n");
			escreveFilas(arq, escalonador->prioridade5);
			fprintf(arq, "\n\t**Prioridade 6**\n\n");
			escreveFilas(arq, escalonador->prioridade6);
			fprintf(arq, "\n\t**Prioridade 7**\n\n");
			escreveFilas(arq, escalonador->prioridade7);
			
			fprintf(arq, "\n\n***********Status das filas de terminados***********\n");
			escreveFilas(arq, escalonador->terminados);
			fprintf(arq, "\n\n***********Status das filas de Em espera***********\n");
			escreveFilas(arq, escalonador->emEspera);

			fprintf(arq, "\n\n***********Programa em execucao***********\n\t%s\n\n\n", escalonador->emExecucao);

			sleep(1);

		}
	}
}
