// convert the contents of stdin to their ASCII values (e.g., 
// '\n' = 10) and spit out the <prog> array used in Figure 1 in
// Thompson's paper.
#include <stdio.h>

int main(void) { 
    // put your code here.
	printf("char prog[] = {\n");
	int count = 0;
	char new_car;
	while((new_car = getchar()) != EOF){
		printf("\t%d,%c", new_car, (count+1)%8==0 ? '\n' : ' ');
		count++;
	}

	printf("0 };\n");
	return 0;
}
