#include "stdio.h"
#include <string.h>
#include <unistd.h> //getpid(),
#include <pthread.h>
#include "stdlib.h"


int a = 0;

void writing_thread(void* arg){
	int pid = getpid();
	printf("[writing thread] pid = %d\n", pid);

	a = a + 10;
	char *ptr;
	ptr = malloc(16);
	printf("[writing thread] a = %d\n", a);
}

void helloFunc(){
	char *ptr;
	ptr = malloc(16);
	printf("hello world\n");
}


int main(int argc, char** argv){
	
	pthread_t id;
	int ret;
	//printf ("ret addr %x\n", &ret);
	int *p;
	p = malloc(4);
	//printf ("p addr %x\n", p);

	ret = pthread_create(&id, NULL, (void *)writing_thread, NULL);
	if(ret != 0){
		printf("[main thread]Create thread failed!\n");
	}

	helloFunc();

	printf("[main thread] a = %d\n", a);

	a = a + 100;

	pthread_join(id,NULL);
	printf("[main thread] writing thread joined\n");


	return 0;
}
