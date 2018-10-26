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
#include "semaforo.h"
#define TEMPOMAX 10
#define N 7

struct no
{
	struct no * prox;
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
	char * progEmExecucao;
	int pidEmExecucao;
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

void insereNoPrioridade (int prioridade, char * nomeProg, Escalonador * escalona, int prioridadeDif, int noComeco)
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
	escalona->emEsperaPr6 = filaCria (-2);
	escalona->emEsperaPr7 = filaCria (-3);
	escalona->progEmExecucao = (char*) malloc (81*sizeof(char));
	strcpy(escalona->progEmExecucao, "nenhum");

	escalona->pidEmExecucao = -1;
	escalona->prioridadeEmExecucao = -1;
	
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
/*
	if (execv((char*)nomeProg, NULL) < 0)
	{
		printf("Nao executou o programa\n");
	}
*/
	printf("%s executando\n", (char*) nomeProg);
}


/*
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

void escalonamento (Escalonador * escalonador, int semId)
{
	int j=0,i;

	clock_t tempoInicialPr6[5],tempoInicialPr7[5], dt[5], dt7[5];

	for(i = 0 ; i < 5 ; i++)
	{
		pthread_create(&threadsIDs[i], NULL, gerenciaFila, (void*) i+1);
		pthread_kill(threadsIDs[i], SIGSTOP);
	}

	for(i=5;i<N;i++)
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
		if(!filaVazia (escalonador->prioridade1)) //REAL TIME
		{
			j=0;

			for(i=1;i<N;i++)
				threadsPausadas[i] = 1;
	
			threadsPausadas[0] = 0;

			if(threadEmExecucao != threadsIDs[0])
				pthread_kill(threadEmExecucao, SIGSTOP);

			threadEmExecucao = threadsIDs[0];
			
			printf("Filho com prioridade 1 executando\n");
			pthread_kill(threadsIDs[0], SIGCONT);
			
		}
		else if( !filaVazia (escalonador->prioridade2) || executando[1] == 1) //REAL TIME
		{
			j=0;

			for(i=2;i<N;i++)
				threadsPausadas[i] = 1;
	
			threadsPausadas[1] = 0;

			if(threadEmExecucao != threadsIDs[1])
				pthread_kill(threadEmExecucao, SIGSTOP);

			threadEmExecucao = threadsIDs[1];

			printf("Filho com prioridade 2 executando\n");
			pthread_kill(threadsIDs[1], SIGCONT);
			
		}
		else if( (!filaVazia (escalonador->prioridade3) || executando[2] == 1) && executando[1] == 0 ) 
		{
			j=0;

			for(i=3;i<N;i++)
				threadsPausadas[i] = 1;
	
			threadsPausadas[2] = 0;

			if(threadEmExecucao != threadsIDs[2])
				pthread_kill(threadEmExecucao, SIGSTOP);

			threadEmExecucao = threadsIDs[2];

			printf("Filho com prioridade 3 executando\n");
			pthread_kill(threadsIDs[2], SIGCONT);
		}
		else if( (!filaVazia (escalonador->prioridade4) || executando[3] == 1) && executando[2] == 0  && executando[1] == 0) 
		{
			j=0;

			for(i=4;i<N;i++)
				threadsPausadas[i] = 1;
	
			threadsPausadas[3] = 0;

			if(threadEmExecucao != threadsIDs[3])
				pthread_kill(threadEmExecucao, SIGSTOP);

			threadEmExecucao = threadsIDs[3];

			printf("Filho com prioridade 4 executando\n");
			pthread_kill(threadsIDs[3], SIGCONT);
			
		}
		else if( (!filaVazia (escalonador->prioridade5) || executando[4] == 1) && executando[3] == 0 && executando[2] == 0  && executando[1] == 0) 
		{
			j=0;

			for(i=5;i<N;i++)
				threadsPausadas[i] = 1;
	
			threadsPausadas[4] = 0;

			if(threadEmExecucao != threadsIDs[4])
				pthread_kill(threadEmExecucao, SIGSTOP);

			threadEmExecucao = threadsIDs[4];

			printf("Filho com prioridade 5 executando\n");
			pthread_kill(threadsIDs[4], SIGCONT);
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
			
		}
		else
		{
			printf(".");
			fflush(stdout);
			j=0;
		}

	}

} 
*/

Fila* retornaFila (Escalonador * escalonador, int prioridade)
{
	switch (prioridade)
	{
		case 1:
			return escalonador->prioridade1;
		case 2:
			return escalonador->prioridade2;
		case 3:
			return escalonador->prioridade3;
		case 4:
			return escalonador->prioridade4;
		case 5:
			return escalonador->prioridade5;
		case 6:
			return escalonador->prioridade6;
		case 7:
			return escalonador->prioridade7;
		default:
			return NULL;
	}
}

void chldHandler (int signal)
{
	//procura qual prioridade acabou -- qual pai recebeu o sinal de que seu filho morreu
	int i;

	printf("Processo %d recebeu sinal de que o filho terminou de executar\n", getpid());
	for(i=0;i<N;i++)
	{
		if(pids[i] == getpid())
		{
			executando[i] = 0;
		}
	}
}

int executaProcesso (char * nomeProg)
{
	int pidFilho, pidNeto, statloc;
	
	signal(SIGCHLD, chldHandler);

	pidFilho = fork();
	if(pidFilho == 0) //vai receber os sinais de STOP e CONT
	{
		pidNeto = fork();
		if(pidNeto == 0) //vai executar
		{
			if (execv(nomeProg, NULL) < 0)
			{
				printf("Nao executou o programa\n");
			}
			exit(0);
		}

		waitpid(-1,&statloc,0);
		printf("Filho mandou sinal de que terminou\n");
		exit(0);
	}
	
	printf("PID pai: %d, PID filho: %d\n", getpid(), pidFilho);
	return pidFilho;
}

void executaFila (Escalonador * escalonador, int prioridade)
{
	char * aux;
	Fila * fila = retornaFila (escalonador,prioridade);

	printf("Prioridade %d executando\n", prioridade);

	if(escalonador->pidEmExecucao > 0) //se tivesse alguém executando
	{
		printf("Parou o processo %d que estava executando\n", escalonador->pidEmExecucao);
		kill(escalonador->pidEmExecucao, SIGSTOP); //para o processo que estava executando
		insereNoPrioridade (escalonador->prioridadeEmExecucao, escalonador->progEmExecucao, escalonador, -1, 1); //reinsere ele no comeco de sua fila
	}

	escalonador->prioridadeEmExecucao = prioridade;
	aux = retiraNo (fila);
	strcpy(escalonador->progEmExecucao, aux);

	if(executando[prioridade-1] == 1) //se ele estivesse executando, continua
	{
		printf("Continuou a execucao de %d\n", pids[prioridade-1]);
		kill(pids[prioridade-1], SIGCONT);
	}
	else //senao, cria novo processo
	{
		pids[prioridade-1] = executaProcesso (aux);
		printf("Comeca a execucao de %d\n", pids[prioridade-1]);
	}
	escalonador->pidEmExecucao = pids[prioridade-1];
}

int gerenciaFilas (Escalonador * escalonador)
{
	int alterou=0,i;

	clock_t tempoInicialPr6[5],tempoInicialPr7[5], dt[5], dt7[5];

	if(!filaVazia (escalonador->prioridade1)) //REAL TIME
	{
		alterou=1;
		executaFila (escalonador, 1);
			
	}
	else if( !filaVazia (escalonador->prioridade2) || executando[1] == 1) //REAL TIME
	{
		alterou=1;
		executaFila (escalonador, 2);
			
	}
	else if( (!filaVazia (escalonador->prioridade3) || executando[2] == 1) && executando[1] == 0 ) 
	{
		alterou=1;
		executaFila (escalonador, 3);
	}
	else if( (!filaVazia (escalonador->prioridade4) || executando[3] == 1) && executando[2] == 0  && executando[1] == 0) 
	{
		alterou=1;
		executaFila (escalonador, 4);
	}
	else if( (!filaVazia (escalonador->prioridade5) || executando[4] == 1) && executando[3] == 0 && executando[2] == 0  && executando[1] == 0) 
	{
		alterou=1;
		executaFila (escalonador, 5);
	}/*
	else if( (!filaVazia (escalonador->prioridade7) || executando[6] == 1 || confereTerminados(terminadosPr7, contPr7, 1) == 0 ) && executando [5] == 0 && executando[4] == 0 && executando[3] == 0 && executando[2] == 0  && executando[1] == 0) //ROUND_ROBIN
	{
		alterou=1;
			

	}*/

	return alterou;
}


void usr1Handler (int signal)
{ 
	novoProcesso = 1;
	printf("Recebi sinal\n");
}

void gerenciaFilaNovos (Escalonador * escalonador)
{
	int numPr = 2810195;
	int numNome = 1808995;
	char * nome;
	int * prioridade;
	int idMemNome, idMemPr;
	//int semId = semget (9950, 1, 0666 | IPC_CREAT);
	//setSemValue(semId);

	signal(SIGUSR1, usr1Handler);


	//while((semId = semget (9950, 1, 0666)) < 0);

	if(novoProcesso == 1)
	{

		idMemNome = shmget(numNome, 81*sizeof(char), IPC_EXCL | S_IRUSR | S_IWUSR);
		nome = (char*) shmat(idMemNome, NULL, 0);	

		idMemPr = shmget(numPr, sizeof(int*), IPC_EXCL | S_IRUSR | S_IWUSR);
		prioridade = (int*) shmat(idMemPr, 0, 0);
		
		insereNoPrioridade (*prioridade, nome , escalonador,-1, 0);

		printf("Inseriu no de prioridade %d\n", *prioridade);

		// libera a memória compartilhada do processo
  		shmdt (nome);
		shmdt (prioridade);

  		// libera a memória compartilhada
  		shmctl (idMemNome, IPC_RMID, 0);
		shmctl (idMemPr, IPC_RMID, 0);

		novoProcesso = 0;

		kill(pidInterpretador, SIGCONT);
	}
}

void escalonamento (Escalonador * escalonador)
{

	int i = 0,j;

	int semId = semget (8860, 1, 0666 | IPC_CREAT);
	setSemValue(semId);

	for(j=0;j<N;j++)	
	{
		executando [j] = 0;
		pids[j] = 0;
	}

	while(i<25)
	{
		semaforoP(semId); //Avisa que vai entrar na região crítica
		printf("Escalonador entrou Regiao Critica\n");
		gerenciaFilaNovos (escalonador);

		if (gerenciaFilas (escalonador) == 0) //se não alterou nada, começa a contagem para encerrar o escalonador
			i++;
		else //se tiver alterado, reseta a contagem
			i=0; 

//		sleep(1);
		
		printf("Escalonador saiu Regiao Critica\n");
		semaforoV(semId); // Avisa que vai sair da região crítica
	}
	escalonamentoTerminou = 1;
	delSemValue(semId);
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
	clock_t dt;

	int semId;

	arq = fopen ("saida.txt","w");
	if (arq == NULL)
	{
		printf("Erro na abertura do arquivo de saida\n");
		exit(1);
	}
	fprintf(arq, "Inicio do escalonamento\n");

	while((semId = semget (8860, 1, 0666)) < 0);

	while(1)
	{
 		i++;
		if (escalonador != NULL)
		{
			semaforoP(semId); //Avisa que vai entrar na região crítica
			printf("STATUS entrou na regiao critica \n");

			dt = (clock() - escalonador->inicio)/CLOCKS_PER_SEC;
			fprintf(arq, "******************** Em t = %.2f ********************\n", (double) dt);
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
			
			fprintf(arq, "\n\n***********Programa em execucao***********\n\t%s---Prioridade: %d\n\n\n", escalonador->progEmExecucao, escalonador->prioridadeEmExecucao);

			if (escalonamentoTerminou == 1)
				break;

			printf("STATUS saiu da regiao critica \n");
			semaforoV(semId); // Avisa que vai sair da região crítica
			
			sleep(1);

		}
	}
	printf("STATUS saiu pra sempre da regiao critica \n");
	semaforoV(semId);
}
