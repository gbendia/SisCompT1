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

int pidsPr6[N];
int pidsPr7[N];

int executandoPr6[N];
int executandoPr7[N];

int contPr6=0;
int contPr7=0;

int atualPr6 = 0;
int atualPr7 = 0;

clock_t dtPr6;
clock_t dtPr7;

clock_t inicioPr6;
clock_t inicioPr7;

int flagPr6 = 0;
int flagPr7 = 0;

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
		case -1:
			return escalonador->terminados;
		case -2:
			return escalonador->emEsperaPr6;
		case -3:
			return escalonador->emEsperaPr7;
		default:
			return NULL;
	}
}

void usr2Handler (int signal)
{

	printf("Processo %d recebeu sinal de que o filho terminou de executar\n", getpid());
	//printf("Prioridade executando: %d \n", escalonador->prioridadeEmExecucao);
	executando[escalonador->prioridadeEmExecucao-1] = 0;

	escalonador->pidEmExecucao = -1;

	insereNoPrioridade (-1, escalonador->progEmExecucao, escalonador, escalonador->prioridadeEmExecucao, 0); //insere na fila de terminados
}

int executaProcesso (char * nomeProg)
{
	int pidFilho, pidNeto, statloc;
	char* vet[] = {nomeProg, NULL}; 

	pidFilho = fork();
	if(pidFilho == 0) //vai receber os sinais de STOP e CONT
	{
		/*pidNeto = fork();
		if(pidNeto == 0) //vai executar
		{
			if (execv(nomeProg, vet) < 0)
			{
				printf("Nao executou o programa\n");
			}
			exit(0);
		}

		waitpid(pidNeto,&statloc,0);
*/
		sleep(8);
		printf("Filho mandou sinal de que terminou\n");
		kill(getppid(), SIGUSR2);
		exit(0);
	}
	
	printf("PID pai: %d, PID filho: %d\n", getpid(), pidFilho);
	return pidFilho;
}

void executaFila (Escalonador * escalonador, int prioridade)
{
	char * aux;
	Fila * fila = retornaFila (escalonador,prioridade);
	int ex = executando[prioridade-1];

	printf("Prioridade %d executando\n", prioridade);
	executando[prioridade-1] = 1;

	if(escalonador->pidEmExecucao > 0 && escalonador->pidEmExecucao != pids[prioridade-1]) //se tivesse alguém executando
	{
		printf("Parou o processo %d que estava executando\n", escalonador->pidEmExecucao);
		kill(escalonador->pidEmExecucao, SIGSTOP); //para o processo que estava executando
		insereNoPrioridade (escalonador->prioridadeEmExecucao, escalonador->progEmExecucao, escalonador, -1, 1); //reinsere ele no comeco de sua fila
	}

	if(escalonador->pidEmExecucao != pids[prioridade-1])
	{
		escalonador->prioridadeEmExecucao = prioridade;
		aux = retiraNo (fila);
		strcpy(escalonador->progEmExecucao, aux);
	}

	if(ex == 1 && escalonador->pidEmExecucao != pids[prioridade-1]) //se ele estivesse executando, continua
	{
		printf("Continuou a execucao de %d\n", pids[prioridade-1]);
		kill(pids[prioridade-1], SIGCONT);
	}
	else //senao, cria novo processo
	{
		if(ex == 0)
		{
			pids[prioridade-1] = executaProcesso (aux);
			printf("Comeca a execucao de %d\n", pids[prioridade-1]);
		}
	}
	escalonador->pidEmExecucao = pids[prioridade-1];
	
}

void executaFilaRoundRobin (Escalonador * escalonador, int prioridade, int prioridadeAnterior, int executaAntigo)
{
	char * aux;
	Fila * fila; 
	int ex = executando[prioridade-1];

	printf("Prioridade %d executando\n", prioridade);
	executando[prioridade-1] = 1;

	if (prioridade != prioridadeAnterior)
		fila = retornaFila (escalonador,prioridade);
	else
	{
		if(prioridade == 6)
			fila = retornaFila (escalonador,-2);
		else
			fila = retornaFila (escalonador,-3);
	}
	
	if(escalonador->pidEmExecucao > 0) //se tivesse alguém executando
	{
		if(prioridade == prioridadeAnterior && executaAntigo == 1)
		{
			printf("Parou o processo %d que estava executando\n", escalonador->pidEmExecucao);
			kill(escalonador->pidEmExecucao, SIGSTOP); //para o processo que estava executando
		}
		if(prioridade != prioridadeAnterior)
		{
			insereNoPrioridade (escalonador->prioridadeEmExecucao, escalonador->progEmExecucao, escalonador, -1, 1); //reinsere ele no comeco de sua fila
		}
		else
		{
			if(prioridade == 6)
				insereNoPrioridade (-2, escalonador->progEmExecucao, escalonador, -1, 0); //reinsere ele no final da fila de espera
			else
				insereNoPrioridade (-3, escalonador->progEmExecucao, escalonador, -1, 0); //reinsere ele no final da fila de espera
		}
	}

	if(escalonador->pidEmExecucao != pids[prioridade-1])
	{
		escalonador->prioridadeEmExecucao = prioridade;
		aux = retiraNo (fila);
		strcpy(escalonador->progEmExecucao, aux);
	}

	if(ex == 1 && escalonador->pidEmExecucao != pids[prioridade-1]) //se ele estivesse executando, continua
	{
		printf("Continuou a execucao de %d\n", pids[prioridade-1]);
		kill(pids[prioridade-1], SIGCONT);
	}
	else //senao, cria novo processo
	{
		if(ex==0)
		{
			pids[prioridade-1] = executaProcesso (aux);
			printf("Comeca a execucao de %d\n", pids[prioridade-1]);
		}
	}
	escalonador->pidEmExecucao = pids[prioridade-1];
}

int gerenciaFilas (Escalonador * escalonador)
{
	int alterou=0;
	int aux;

	if(!filaVazia (escalonador->prioridade1)) //REAL TIME
	{
		alterou=1;
		executaFila (escalonador, 1);
			
	}
	else if( (!filaVazia (escalonador->prioridade2) || executando[1] == 1) && executando[0] == 0) //REAL TIME
	{
		alterou=1;
		executaFila (escalonador, 2);
			
	}
	else if( (!filaVazia (escalonador->prioridade3) || executando[2] == 1) && executando[0] == 0 && executando[1] == 0 ) 
	{
		alterou=1;
		executaFila (escalonador, 3);
	}
	else if( (!filaVazia (escalonador->prioridade4) || executando[3] == 1) && executando[0] == 0 && executando[1] == 0  && executando[2] == 0) 
	{
		alterou=1;
		executaFila (escalonador, 4);
	}
	else if( (!filaVazia (escalonador->prioridade5) || executando[4] == 1) && executando[0] == 0 && executando[1] == 0 && executando[2] == 0  && executando[3] == 0) 
	{
		alterou=1;
		executaFila (escalonador, 5);
	}
	else if( (!filaVazia (escalonador->prioridade6) || executando[5] == 1) && executando[0] == 0 && executando[1] == 0 && executando[2] == 0  && executando[3] == 0  && executando[4] == 0) //ROUND_ROBIN
	{
		alterou=1;
		executaFilaRoundRobin (escalonador, 6, escalonador->prioridadeEmExecucao,6);
	}
	else if( (!filaVazia (escalonador->prioridade7) || executando[6] == 1) && executando[0] == 0 && executando[1] == 0 && executando[2] == 0  && executando[3] == 0  && executando[4] == 0 && executando [5] == 0 ) //ROUND_ROBIN
	{
		alterou=1;
		if(flagPr7 == 0)
		{
			inicioPr7 = clock();
			flagPr7 = 1;
		}
		if(escalonador->prioridadeEmExecucao == 7)
		{
			dtPr7 = clock() - inicioPr7;
			if(dtPr7 > 3)
			{
				flagPr7 == 0;
				pids[6] = pidsPr7[atualPr7];
				aux = executando[6];
				executando[6] = executandoPr7[atualPr7];
				executaFilaRoundRobin (escalonador, 7, escalonador->prioridadeEmExecucao, aux);
				pids[atualPr7] = pidsPr7[6];
				atualPr7++;
				if(atualPr7 > contPr7)
					atualPr7 = 0;
			}
		}
		else
		{
			flagPr7 = 0;
			executaFilaRoundRobin (escalonador, 7, escalonador->prioridadeEmExecucao, executando[6]);
		}
	}

	return alterou;
}


void usr1Handler (int signal)
{ 
	novoProcesso = 1;
//	printf("Recebi sinal\n");
}

void gerenciaFilaNovos (Escalonador * escalonador)
{
	int numPr = 3956543;
	int numNome = 4354567;
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

		novoProcesso = 0;

		kill(pidInterpretador, SIGCONT);
	}
}

void escalonamento (Escalonador * escalonador)
{

	int i = 0,j;

	int semId = semget (32345678, 1, 0666 | IPC_CREAT);
	setSemValue(semId);
	
	signal(SIGUSR2, usr2Handler);
	for(j=0;j<N;j++)	
	{
		executando [j] = 0;
		pids[j] = 0;
		executandoPr6 [j] = 0;
		pidsPr6[j] = 0;
		executandoPr7 [j] = 0;
		pidsPr7[j] = 0;
	}

	while(i<20)
	{
		semaforoP(semId); //Avisa que vai entrar na região crítica
//		printf("Escalonador entrou Regiao Critica\n");
		gerenciaFilaNovos (escalonador);

		if (gerenciaFilas (escalonador) == 0) //se não alterou nada, começa a contagem para encerrar o escalonador
			i++;
		else //se tiver alterado, reseta a contagem
			i=0; 

		sleep(1);
		
//		printf("Escalonador saiu Regiao Critica\n");
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
//	clock_t dt;

	int semId;

	arq = fopen ("saida.txt","w");
	if (arq == NULL)
	{
		printf("Erro na abertura do arquivo de saida\n");
		exit(1);
	}
	fprintf(arq, "Inicio do escalonamento\n");

	while((semId = semget (32345678, 1, 0666)) < 0);

	while(1)
	{
 		i++;
		if (escalonador != NULL)
		{
			semaforoP(semId); //Avisa que vai entrar na região crítica
//			printf("STATUS entrou na regiao critica \n");

//			dt = (clock() - escalonador->inicio)/CLOCKS_PER_SEC;
//			fprintf(arq, "******************** Em t = %.2f ********************\n", (double) dt);
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

//			printf("STATUS saiu da regiao critica \n");
			semaforoV(semId); // Avisa que vai sair da região crítica
			
			sleep(1);

		}
	}
	printf("STATUS saiu pra sempre da regiao critica \n");
	semaforoV(semId);
}

/*

[obi.grad.inf.puc-rio.br:~/Documents/SIsComp/Trabalho1/OutraVersao] ./Trab
Inseriu no de prioridade 7
Prioridade 7 executando
PID pai: 6280, PID filho: 6283
Comeca a execucao de 6283
Inseriu no de prioridade 2
Prioridade 2 executando
Parou o processo 6283 que estava executando
PID pai: 6280, PID filho: 6284
Comeca a execucao de 6284
Inseriu no de prioridade 7
Prioridade 2 executando
Inseriu no de prioridade 1
Prioridade 1 executando
Parou o processo 6284 que estava executando
PID pai: 6280, PID filho: 6286
Comeca a execucao de 6286
Interpretador terminou
Filho mandou sinal de que terminou
Processo 6280 recebeu sinal de que o filho terminou de executar
Prioridade 2 executando
Continuou a execucao de 6284
Filho mandou sinal de que terminou
Processo 6280 recebeu sinal de que o filho terminou de executar
Prioridade 7 executando
Continuou a execucao de 6283
Filho mandou sinal de que terminou
Processo 6280 recebeu sinal de que o filho terminou de executar
Prioridade 7 executando
Segmentation fault (core dumped)

*/
