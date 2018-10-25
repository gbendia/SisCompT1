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
	int pid;
	int prioridade;
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
	Fila * emEsperaPr6;
	Fila * emEsperaPr7;
	char * emExecucao;
	int prioridadeEmExecucao;

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

void insereNoPrioridade (int prioridade, char * nomeProg, Escalonador * escalona, int prioridadeDif, int pid)
{
	No * novo = (No*) malloc (sizeof(No));
	Fila * aux;
	if (novo == NULL)
	{
		printf("Erro na criacao do no do programa %s\n", nomeProg);
		exit(1);
	}

	strcpy(novo->nomeProg, nomeProg);

	if(prioridadeDif < 0)
		novo->prioridade = prioridade;
	else
		novo->prioridade = prioridadeDif;
		
	novo->prox = NULL;
	novo->pid = pid;

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
			aux = escalona->emEsperaPr6;
			break;
		case -3:
			aux = escalona->emEsperaPr7;
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

char * retiraNo (Fila * fila, int * pid)
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
		*pid = fila->primeiro->pid;
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
	escalona->emEsperaPr6 = filaCria (-2);
	escalona->emEsperaPr7 = filaCria (-3);
	escalona->emExecucao = (char*) malloc (81*sizeof(char));
	strcpy(escalona->emExecucao, "nenhum");

	escalona->prioridadeEmExecucao = 0;

	
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

void thread_executa (void *nomeProg)
{
	if (execv((char*)nomeProg, NULL) < 0)
	{
		printf("Nao executou o programa\n");
	}
}

void gerenciaFila (void *t)
{
	pthread_t threadExecuta[1];
	Fila* aux;
	char * nomeProg
	int prioridade = (int) t;
	prioridade ++;
	sleep(1);
	switch(prioridade)
	{
		case 1:
			aux = escalonador->prioridade1;
			break;
		case 2:
			aux = escalonador->prioridade2;
			break;
		case 3:
			aux = escalonador->prioridade3;
			break;
		case 4:
			aux = escalonador->prioridade4;
			break;
		case 5:
			aux = escalonador->prioridade5;
			break;
		default:
			aux = NULL;
			break
	}

	while ( !filaVazia(aux) )
	{
		nomeProg = retiraNo (aux);
		strcpy(escalonador->emExecucao,nomeProg);
		printf("Thread de prioridade %d executando %s \n", prioridade, nomeProg);
		executando[prioridade - 1] = 1;

		if(threadsPausadas[prioridade - 1] == 1)
			pthread_create(&threadExecuta[0], NULL, thread_executa, (void*) nomeProg);

		pthread_join(threadExecuta[0],NULL); // espera a thread que executa terminar
		executando[prioridade - 1] = 0;
		insereNoPrioridade (-1, nomeProg, escalonador, prioridade); // inserir na fila de terminados
	}
}

void manejaFila (int num, Escalonador * escalonador, Fila *aux, int * pid, int * executando, char ** nomeProg, int * pausados, int * terminados)
{
	int pidAux;
	if(executando[num] == 0)
	{
		printf("Filho com prioridade %d executando\n", aux->prioridade);
		nomeProg[num] = retiraNo (aux, &pidAux);
		printf("Nome do programa a ser executado: %s\n", nomeProg[num]);
		strcpy(escalonador->emExecucao,nomeProg[num]);
		escalonador->prioridadeEmExecucao = aux->prioridade;
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
		insereNoPrioridade (aux->prioridade, nomeProg[num], escalonador, -1, pid[num]); // inserir na fila de sua prioridade
	}
	else
	{
		if(pausados[num] == 1 && terminados[num] == 0)
		{
			printf("Programa %s despausado\n", nomeProg[num]);
			kill (pid[num], SIGCONT);
			printf("Conseguiu dar SIGCONT em %d\n",pid[num]);
			retiraNo (aux,&pidAux);
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
		insereNoPrioridade (-1, nomeProg[num], escalonador, aux->prioridade, pid[num]); // inserir na fila de terminados
		strcpy(escalonador->emExecucao,"nenhum");
		escalonador->prioridadeEmExecucao = 0;
		printf("Inseriu %s na fila de terminados\n", nomeProg[num]);
	}
}

void escalonamento (Escalonador * escalonador)
{
	int j=0,i;

	clock_t tempoInicialPr6[5],tempoInicialPr7[5], dt[5], dt7[5];

	for(i = 0 ; i < N ; i++)
	{
		pthread_create(&threadsIDs[i], NULL, gerenciaFilaRoundRobin, (void*) i+1);
		pthread_kill(threadsIDs[i], SIGSTOP);
	}

	sleep(1);
	printf("Comeco do escalonamento\n");

	for(i=0;i<N;i++)
		threadsPausadas[i] = 1;	

	while(j < 100)
	{
		if( !filaVazia (escalonador->prioridade1) && threadsPausadas[1] && threadsPausadas[2] && threadsPausadas[3] && threadsPausadas[4] && threadsPausadas[5] && threadsPausadas[6] ) //REAL TIME
		{
			j=0;
			printf("Filho com prioridade 1 executando\n");
			pthread_kill(threadsIDs[0], SIGCONT);
			
		}
		else if( !filaVazia (escalonador->prioridade2) || executando[1] == 1) //REAL TIME
		{
			j=0;

			printf("Filho com prioridade 2 executando\n");
			pthread_kill(threadsIDs[1], SIGCONT);
			
		}
		else if( (!filaVazia (escalonador->prioridade3) || executando[2] == 1) && executando[1] == 0 ) 
		{
			j=0;
			gerenciaFila (2, escalonador, escalonador->prioridade3, pid, executando, nomeProg, pausados, terminados);
		}
		else if( (!filaVazia (escalonador->prioridade4) || executando[3] == 1) && executando[2] == 0  && executando[1] == 0) 
		{
			j=0;
			gerenciaFila (3, escalonador, escalonador->prioridade4, pid, executando, nomeProg, pausados, terminados);
			
		}
		else if( (!filaVazia (escalonador->prioridade5) || executando[4] == 1) && executando[3] == 0 && executando[2] == 0  && executando[1] == 0) 
		{
			j=0;
			gerenciaFila (4, escalonador, escalonador->prioridade5, pid, executando, nomeProg, pausados, terminados);
		}
			else if( (!filaVazia (escalonador->prioridade6) || executando[5] == 1 || confereTerminados(terminadosPr6, contPr6, 1) == 0 ) && executando[4] == 0 && executando[3] == 0 && executando[2] == 0  && executando[1] == 0) //ROUND_ROBIN
		{
			j=0;
			if(executando[5] == 0 || (contPr6 < 5 && flagPr6 == 0) && !filaVazia (escalonador->prioridade6))
			{
				printf("Filho com prioridade %d executando\n", escalonador->prioridade6->prioridade);
				nomeProgPr6[contPr6] = retiraNo (escalonador->prioridade6, &pidAux);
				printf("Nome do programa a ser executado: %s\n", nomeProgPr6[contPr6]);
				strcpy(escalonador->emExecucao,nomeProgPr6[contPr6]);
				escalonador->prioridadeEmExecucao = 6;
				pausados[5] = 0;
				pidPr6[contPr6] = fork();
				emExecPr6 = contPr6;
				if(pidPr6[contPr6]==0) // filho executa o programa
				{
					printf("Está no processo filho %d\n",getpid());

					//if (execv(nomeProg, NULL) < 0)
				//	{
				//		printf("Nao executou o programa\n");
				//	}
					sleep(8);
					
					terminados[5] = 1;
					terminadosPr6[contPr6] = 1;

					printf("Filho %d terminou sua execucao\n", getpid());
					exit(0);
				}
				contPr6 ++;
				flagPr6 = 1;
				executando[5] = 1;
			}
			
			if(checaPrioridade(escalonador, escalonador->prioridade6->prioridade) == 1)
			{
				printf("Programa %s pausado\n", nomeProgPr6[emExecPr6]);
				kill (pidPr6[emExecPr6], SIGSTOP);
				printf("Conseguiu dar SIGSTOP em %d\n",pidPr6[emExecPr6]);
				pausados[5] = 1;
				insereNoPrioridade (6, nomeProgPr6[emExecPr6], escalonador,-1,pidPr6[emExecPr6]); 
			}
			else
			{
				printf("b\n");
				printf("%d ----- %d ------- %d\n",!filaVazia (escalonador->prioridade6), executando[5] == 1, confereTerminados(terminadosPr6, contPr6, 1) ==0);
				if(terminados[5] == 0)
				{
					if(pausados[5] == 1)
					{
						nomeAux = retiraNo (escalonador->prioridade6,&pidAux);
						if(nomeAux != NULL)
						{
							printf("Programa %s despausado\n", nomeProgPr6[emExecPr6]);
							kill (pidPr6[emExecPr6], SIGCONT);
							printf("Conseguiu dar SIGCONT em %d\n",pidPr6[emExecPr6]);
						
							printf("Retirou o nó\n");
							pausados[5] = 0;
						}
					}
					else
					{
						for(k=iniPr6;k<contPr6;k++)
						{
							kill (pidPr6[k], SIGCONT);
							//printf("ROUND_ROBIN: Conseguiu dar SIGCONT em %d\n",pidPr6[k]);
							if(inicialPr6[k] == 0)
							{
								sleep(1);
								kill (pidPr6[k], SIGSTOP);
								printf("ROUND_ROBIN: Conseguiu dar SIGSTOP em %d\n",pidPr6[k]);
								insereNoPrioridade (-2, nomeProgPr6[k], escalonador, 6, pidPr6[k]); // inserir na fila de espera
								strcpy(escalonador->emExecucao,"nenhum");
								escalonador->prioridadeEmExecucao = 0;
								tempoInicialPr6[k] = clock();
								inicialPr6[k] = 1;
								flagPr6 = 0;
							}
						}
					}
				}
			}

			while(!filaVazia(escalonador->emEsperaPr6))
			{
				retiraNo (escalonador->emEsperaPr6,&pidAux);
				pos = buscaPid (pidPr6, pidAux);
				if(flagPr6Cont == 0)
				{
					dt[pos] = (clock() - tempoInicialPr6[pos])/CLOCKS_PER_SEC; //nao podiamos colocar sleep(3) pois travaria a execucao das outras filas
					if((double)dt[pos] > 3)
					{
						inicialPr6[pos] = 0;
						kill (pidAux, SIGCONT);
						printf("ROUND_ROBIN: Conseguiu dar SIGCONT em %d\n",pidAux);
						flagPr6Cont = 1;
						emExecPr6 = pos;
					}
					flagPr6Cont = 1;
				}
			}
			for(m=0;m<5;m++)
			{
				if(terminadosPr6[m] == 1)
				{
					printf("Filho %d terminou de executar\n", pidPr6[m]);
					terminadosPr6[m] = 0;
					if(confereTerminados(terminadosPr6, contPr6, 0) == 1 )
					{
						executando[5] = 0;
						pausados[5] = 0;
						terminados[5] = 1;
						for(n=0;n<5;n++) terminadosPr6[n] = 0;
					}
					pausados[5] = 1;

					insereNoPrioridade (-1, nomeProgPr6[m], escalonador, 6,pidPr6[m]); // inserir na fila de terminados
					strcpy(escalonador->emExecucao,"nenhum");
					escalonador->prioridadeEmExecucao = 0;
					printf("Inseriu %s na fila de terminados\n", nomeProgPr6[m]);
					flagPr6Cont = 0;
					iniPr6 ++;
				}
			}			
			
		}
		else if( (!filaVazia (escalonador->prioridade7) || executando[6] == 1 || confereTerminados(terminadosPr7, contPr7, 1) == 0 ) && executando [5] == 0 && executando[4] == 0 && executando[3] == 0 && executando[2] == 0  && executando[1] == 0) //ROUND_ROBIN
		{
			j=0;
			if(executando[6] == 0 || (contPr7 < 5 && flagPr7 == 0) && !filaVazia (escalonador->prioridade7))
			{
				printf("Filho com prioridade %d executando\n", escalonador->prioridade7->prioridade);
				nomeProgPr7[contPr7] = retiraNo (escalonador->prioridade7, &pidAux);
				printf("Nome do programa a ser executado: %s\n", nomeProgPr7[contPr7]);
				strcpy(escalonador->emExecucao,nomeProgPr7[contPr7]);
				escalonador->prioridadeEmExecucao = 7;
				executando[6] = 1;
				pausados[6] = 0;
				pidPr7[contPr7] = fork();
				emExecPr7 = contPr7;
				if(pidPr7[contPr7]==0) // filho executa o programa
				{
					printf("Está no processo filho %d\n",getpid());

					//if (execv(nomeProg, NULL) < 0)
				//	{
				//		printf("Nao executou o programa\n");
				//	}
					sleep(8);
					
					terminados[6] = 1;
					terminadosPr7[contPr7] = 1;

					printf("Filho %d terminou sua execucao\n", getpid());
					exit(0);
				}
				contPr7 ++;
				flagPr7 = 1;
			}
			
			if(checaPrioridade(escalonador, escalonador->prioridade7->prioridade) == 1)
			{
				printf("Programa %s pausado\n", nomeProgPr7[emExecPr7]);
				kill (pidPr7[emExecPr7], SIGSTOP);
				printf("Conseguiu dar SIGSTOP em %d\n",pidPr7[emExecPr7]);
				pausados[6] = 1;
				insereNoPrioridade (7, nomeProgPr7[emExecPr7], escalonador,-1,pidPr7[emExecPr7]); 
			}
			else
			{
				if(terminados[6] == 0)
				{
					if(pausados[6] == 1)
					{
						nomeAux = retiraNo (escalonador->prioridade7,&pidAux);
						if(nomeAux != NULL)
						{
							printf("Programa %s despausado\n", nomeProgPr7[emExecPr7]);
							kill (pidPr7[emExecPr7], SIGCONT);
							printf("Conseguiu dar SIGCONT em %d\n",pidPr7[emExecPr7]);
						
							printf("Retirou o nó\n");
							pausados[6] = 0;
						}
					}
					else
					{
						for(k=iniPr7;k<contPr7;k++)
						{
							kill (pidPr7[k], SIGCONT);
							//printf("ROUND_ROBIN: Conseguiu dar SIGCONT em %d\n",pidPr6[k]);
							if(inicialPr7[k] == 0)
							{
								sleep(1);
								kill (pidPr7[k], SIGSTOP);
								printf("ROUND_ROBIN: Conseguiu dar SIGSTOP em %d\n",pidPr7[k]);
								insereNoPrioridade (-3, nomeProgPr7[k], escalonador, 7, pidPr7[k]); // inserir na fila de espera
								strcpy(escalonador->emExecucao,"nenhum");
								escalonador->prioridadeEmExecucao = 0;
								tempoInicialPr7[k] = clock();
								inicialPr7[k] = 1;
								flagPr7 = 0;
							}
						}
					}
				}
			}

			while(!filaVazia(escalonador->emEsperaPr7))
			{
				retiraNo (escalonador->emEsperaPr7,&pidAux);
				pos7 = buscaPid (pidPr7, pidAux);
				if(flagPr7Cont == 0)
				{
					dt7[pos7] = (clock() - tempoInicialPr7[pos7])/CLOCKS_PER_SEC; //nao podiamos colocar sleep(3) pois travaria a execucao das outras filas
					if((double)dt7[pos7] > 3)
					{
						inicialPr7[pos7] = 0;
						kill (pidAux, SIGCONT);
						printf("ROUND_ROBIN: Conseguiu dar SIGCONT em %d\n",pidAux);
						flagPr7Cont = 1;
						emExecPr7 = pos7;
					}
					flagPr7Cont = 1;
				}
			}
			for(m=0;m<5;m++)
			{
				if(terminadosPr7[m] == 1)
				{
					printf("Filho %d terminou de executar\n", pidPr7[m]);
					terminadosPr7[m] = 0;
					terminados[6] = 0;
					if(confereTerminados(terminadosPr7, contPr7, 0) == 1 )
					{
						executando[6] = 0;
						pausados[6] = 0;
					}
					pausados[6] = 1;

					insereNoPrioridade (-1, nomeProgPr7[m], escalonador, 7,pidPr7[m]); // inserir na fila de terminados
					strcpy(escalonador->emExecucao,"nenhum");
					escalonador->prioridadeEmExecucao = 0;
					printf("Inseriu %s na fila de terminados\n", nomeProgPr7[m]);
					flagPr7Cont = 0;
					iniPr7 ++;
				}
			}
		}
		else
		{
			printf(".");
			fflush(stdout);
			j++;
		}

	}

	for(i = 0; i < N; i++) {
		pthread_join(threads[i],NULL); // Espera todas as threads terminarem
	}	

}

void gerenciaFilaNovos (Escalonador * escalonador)
{
	int numPr = 8850101;
	int numNome = 3122422;
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
		
		insereNoPrioridade (*prioridade, nome , escalonador,-1,0);

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
			fprintf(arq, "\tPrograma %s -- Prioridade: %d\n", aux->nomeProg, aux->prioridade);
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

	while(i<40)
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
			fprintf(arq, "\n\t**Prioridade 4**\n\n");
			escreveFilas(arq, escalonador->prioridade4);
			fprintf(arq, "\n\t**Prioridade 5**\n\n");
			escreveFilas(arq, escalonador->prioridade5);
			fprintf(arq, "\n\t**Prioridade 6**\n\n");
			escreveFilas(arq, escalonador->prioridade6);
			fprintf(arq, "\n\t**Prioridade 7**\n\n");
			escreveFilas(arq, escalonador->prioridade7);
			
			fprintf(arq, "\n\n***********Status das filas de terminados***********\n");
			escreveFilas(arq, escalonador->terminados);
			fprintf(arq, "\n\n***********Status das filas de Em espera da Prioridade 6***********\n");
			escreveFilas(arq, escalonador->emEsperaPr6);
			fprintf(arq, "\n\n***********Status das filas de Em espera da Prioridade 7***********\n");
			escreveFilas(arq, escalonador->emEsperaPr7);
			
			fprintf(arq, "\n\n***********Programa em execucao***********\n\t%s---Prioridade: %d\n\n\n", escalonador->emExecucao, escalonador->prioridadeEmExecucao);

			sleep(1);

		}
	}
}
