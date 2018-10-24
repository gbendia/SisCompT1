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
	char * emExecucao;

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
		case -2:
			aux = escalona->emEspera;
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


	if(fila == NULL)
	{
		return NULL;
	}

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
	escalona->emExecucao = (char*) malloc (81*sizeof(char));
	strcpy(escalona->emExecucao, "nenhum");

	
	escalona->inicio = clock();

	return escalona;
}

void zeraVetor(int * vet, int n)
{
	int i;
	for(i=0;i<n;i++)	
	{
		vet[i] = 0;
	}
}

int checaPrioridade(Escalonador * escalonador, int prioridade)
{
	if(!filaVazia (escalonador->prioridade1))
		return 1;
	if(prioridade != 2 && !filaVazia (escalonador->prioridade2))
		return 1;
	if(prioridade != 2 && prioridade != 3 && !filaVazia (escalonador->prioridade3))
		return 1;
	if(prioridade != 2 && prioridade != 3 && prioridade != 4 && !filaVazia (escalonador->prioridade4))
		return 1;
	if(prioridade != 2 && prioridade != 3 && prioridade != 4 && prioridade != 5 && !filaVazia (escalonador->prioridade5))
		return 1;
	if(prioridade != 2 && prioridade != 3 && prioridade != 4 && prioridade != 5 && prioridade != 6 && !filaVazia (escalonador->prioridade6))
		return 1;
	return 0;
}

void manejaFila (int num, Escalonador * escalonador, Fila *aux, int * pid, int * executando, char ** nomeProg, int * pausados, int * terminados)
{
	if(executando[num] == 0)
	{
		printf("Filho com prioridade %d executando\n", aux->prioridade);
		nomeProg[num] = retiraNo (aux);
		printf("Nome do programa a ser executado: %s\n", nomeProg[num]);
		strcpy(escalonador->emExecucao,nomeProg[num]);
		executando[num] = 1;
		pausados[num] = 0;
		pid[num] = fork();
		if(pid[num]==0) // filho executa o programa
		{
			printf("Está no processo filho %d\n",getpid());

			//if (execv(nomeProg, NULL) < 0)
		//	{
		//		printf("Nao executou o programa\n");
				//	}
			sleep(4);
					
			printf("Filho %d alterou a variavel terminados\n", getpid());
			terminados[num] = 1;
			exit(0);
		}
	}
			
	if(checaPrioridade(escalonador, aux->prioridade) == 1)
	{
		printf("Programa %s pausado\n", nomeProg[num]);
		kill (pid[num], SIGSTOP);
		printf("Conseguiu dar SIGSTOP em %d\n",pid[num]);
		pausados[num] = 1;
		insereNoPrioridade (aux->prioridade, nomeProg[num], escalonador); // inserir na fila de espera
	}
	else
	{
		if(pausados[num] == 1 && terminados[num] == 0)
		{
			printf("Programa %s despausado\n", nomeProg[num]);
			kill (pid[num], SIGCONT);
			printf("Conseguiu dar SIGCONT em %d\n",pid[num]);
			retiraNo (aux);
			printf("Retirou o nó\n");
			pausados[num] = 0;
		}
	}
	if(terminados[num] == 1)
	{
		printf("Filho %d terminou de executar\n", pid[num]);
		terminados[num] = 0;
		executando[num] = 0;
		pausados[num] = 1;
		insereNoPrioridade (-1, nomeProg[num], escalonador); // inserir na fila de terminados
		strcpy(escalonador->emExecucao,"nenhum");
		printf("Inseriu %s na fila de terminados\n", nomeProg[num]);
	}
}

void escalonamento (Escalonador * escalonador)
{
	int pausados [] = {1,1,1,1,1,1,1};
	int executando [] = {0,0,0,0,0,0,0};
	int *terminados;
	int pid [7];
	int j=0;
	char *nomeProg[7];
	int statloc;

	int numId = shmget(IPC_PRIVATE, 7*sizeof(int) , IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	
	terminados = (int*) shmat(numId, 0, 0); 
	zeraVetor(terminados, 7);

	sleep(1);
	printf("Comeco escalonamento\n");

	while(j < 50)
	{
		if( !filaVazia (escalonador->prioridade1) && pausados[1] && pausados[2] && pausados[3] && pausados[4] && pausados[5] && pausados[6] ) //REAL TIME
		{
			j=0;
			printf("Filho com prioridade %d executando\n", escalonador->prioridade1->prioridade);
			nomeProg[0] = retiraNo (escalonador->prioridade1);
			strcpy(escalonador->emExecucao,nomeProg[0]);
			pid[0] = fork();
			if(pid[0]==0) // filho executa o programa
			{
				if (execv(nomeProg[0], NULL) < 0)
				{
					printf("Nao executou o programa\n");
				}
				
				exit(0);
			}
			else // pai espera ele terminar, pois é prioridade 1
			{
				waitpid(-1,&statloc,0);
				printf("Filho terminou de executar\n");

				insereNoPrioridade (-1, nomeProg[0], escalonador); // inserir na fila de terminados
				strcpy(escalonador->emExecucao,"nenhum");
				//printf("Inseriu na fila de terminados\n");
			}
		}
		else if( !filaVazia (escalonador->prioridade2) || executando[1] == 1) //REAL TIME
		{
			j=0;
			
			manejaFila (1, escalonador, escalonador->prioridade2, pid, executando, nomeProg, pausados, terminados);
		}
		else if( (!filaVazia (escalonador->prioridade3) || executando[2] == 1) && executando[1] == 0 ) 
		{
			j=0;
			manejaFila (2, escalonador, escalonador->prioridade3, pid, executando, nomeProg, pausados, terminados);
		}
		else if( (!filaVazia (escalonador->prioridade4) || executando[3] == 1) && executando[2] == 0  && executando[1] == 0) 
		{
			j=0;
			manejaFila (3, escalonador, escalonador->prioridade4, pid, executando, nomeProg, pausados, terminados);
			
		}
		else if( (!filaVazia (escalonador->prioridade5) || executando[4] == 1) && executando[3] == 0 && executando[2] == 0  && executando[1] == 0) 
		{
			j=0;
			manejaFila (4, escalonador, escalonador->prioridade5, pid, executando, nomeProg, pausados, terminados);
		}
		else if( (!filaVazia (escalonador->prioridade6) || executando[5] == 1) && executando[4] == 0 && executando[3] == 0 && executando[2] == 0  && executando[1] == 0) //ROUND_ROBIN
		{
			j=0;
			if(executando[5] == 0)
			{
				printf("Filho com prioridade %d executando\n", escalonador->prioridade6->prioridade);
				nomeProg[5] = retiraNo (escalonador->prioridade6);
				printf("Nome do programa a ser executado: %s\n", nomeProg[5]);
				strcpy(escalonador->emExecucao,nomeProg[5]);
				executando[5] = 1;
				pausados[5] = 0;
				pid[5] = fork();
				if(pid[5]==0) // filho executa o programa
				{
					printf("Está no processo filho %d\n",getpid());

					//if (execv(nomeProg, NULL) < 0)
				//	{
				//		printf("Nao executou o programa\n");
				//	}
					sleep(4);
					
					printf("Filho alterou a variavel terminados\n");
					terminados[5] = 1;
					exit(0);
				}
			}
			
			if(!filaVazia (escalonador->prioridade1) && !filaVazia (escalonador->prioridade2) && !filaVazia (escalonador->prioridade3) && !filaVazia (escalonador->prioridade4) && !filaVazia (escalonador->prioridade5))
			{
				printf("Programa %s pausado\n", nomeProg[5]);
				kill (pid[5], SIGSTOP);
				printf("Conseguiu dar SIGSTOP em %d\n",pid[5]);
				pausados[5] = 1;
				insereNoPrioridade (-2, nomeProg[5], escalonador); // inserir na fila de espera
			}
			else
			{
				if(terminados[5] == 0)
				{
					if(pausados[5] == 1)
					{
						printf("Programa %s despausado\n", nomeProg[5]);
						kill (pid[5], SIGCONT);
						printf("Conseguiu dar SIGCONT em %d\n",pid[5]);
						retiraNo (escalonador->emEspera);
						printf("Retirou o nó\n");
						pausados[5] = 0;
					}
					else
					{
						sleep(1);
						kill (pid[5], SIGSTOP);
						printf("ROUND_ROBIN: Conseguiu dar SIGSTOP em %d\n",pid[5]);
					}
				}
			}
			if(terminados[5] == 1)
			{
				printf("Filho %d terminou de executar\n", pid[5]);
				terminados[5] = 0;
				executando[5] = 0;
				pausados[5] = 1;

				insereNoPrioridade (-1, nomeProg[5], escalonador); // inserir na fila de terminados
				strcpy(escalonador->emExecucao,"nenhum");
				printf("Inseriu %s na fila de terminados\n", nomeProg[5]);
			}
			
		}
		else if( (!filaVazia (escalonador->prioridade7) || executando[6] == 1) && executando [5] == 0 && executando[4] == 0 && executando[3] == 0 && executando[2] == 0  && executando[1] == 0) //ROUND_ROBIN
		{
			printf("Fila de prioridade 7: num elem: %d\n", escalonador->prioridade7->qntElementos);
			j=0;

			if(executando[6] == 0)
			{
				printf("Filho com prioridade %d executando\n", escalonador->prioridade7->prioridade);
				nomeProg[6] = retiraNo (escalonador->prioridade7);
				printf("Nome do programa a ser executado: %s\n", nomeProg[6]);
				strcpy(escalonador->emExecucao,nomeProg[5]);
				executando[5] = 1;
				pausados[5] = 0;
				pid[5] = fork();
				if(pid[5]==0) // filho executa o programa
				{
					printf("Está no processo filho %d\n",getpid());

					//if (execv(nomeProg, NULL) < 0)
				//	{
				//		printf("Nao executou o programa\n");
				//	}
					sleep(4);
					
					printf("Filho alterou a variavel terminados\n");
					terminados[5] = 1;
					exit(0);
				}
			}
			
			if(!filaVazia (escalonador->prioridade1) && !filaVazia (escalonador->prioridade2) && !filaVazia (escalonador->prioridade3) && !filaVazia (escalonador->prioridade4) && !filaVazia (escalonador->prioridade5))
			{
				printf("Programa %s pausado\n", nomeProg[5]);
				kill (pid[5], SIGSTOP);
				printf("Conseguiu dar SIGSTOP em %d\n",pid[5]);
				pausados[5] = 1;
				insereNoPrioridade (-2, nomeProg[5], escalonador); // inserir na fila de espera
			}
			else
			{
				if(pausados[5] == 1 && terminados[5] == 0)
				{
					printf("Programa %s despausado\n", nomeProg[5]);
					kill (pid[5], SIGCONT);
					printf("Conseguiu dar SIGCONT em %d\n",pid[5]);
					retiraNo (escalonador->emEspera);
					printf("Retirou o nó\n");
					pausados[5] = 0;
				}
			}
			if(terminados[5] == 1)
			{
				printf("Filho %d terminou de executar\n", pid[5]);
				terminados[5] = 0;
				executando[5] = 0;
				pausados[5] = 1;

				insereNoPrioridade (-1, nomeProg[5], escalonador); // inserir na fila de terminados
				strcpy(escalonador->emExecucao,"nenhum");
				printf("Inseriu %s na fila de terminados\n", nomeProg[5]);
			}
		}
		else
			j++;
	}
}

void gerenciaFilaNovos (Escalonador * escalonador)
{
	int numPr = 98765;
	int numNome = 10203040;
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

	while(i<20)
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
