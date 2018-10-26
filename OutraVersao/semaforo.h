#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>

union semun
{
	int val;
	struct semid_ds *buf;
	ushort * array;
};
// inicializa o valor do semáforo
int setSemValue(int semId);
// remove o semáforo
void delSemValue( int semId);
// operação P
int semaforoP(int semId);
//operação V
int semaforoV(int semId);
