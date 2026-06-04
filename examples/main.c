#include <stdio.h>

int main(void){
	char *msg = (char *)main;

	int *x = (int *)0;
	printf("%d\n", *x);

	*x = 4096;
	printf("%d\n", *x);

	msg[0] = 'H';
	msg[1] = 'i';
	msg[2] = '\0';
	
	printf("%s\n", msg);
	printf("i'm so smart :3\n");

	return 0;
}
