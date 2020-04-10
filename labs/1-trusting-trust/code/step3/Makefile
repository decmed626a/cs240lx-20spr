# you should not need to modify this.
CC =gcc
CFLAGS = -Og -g -Wall -Wno-unused-variable

all: string-to-char-array clone

clone:
	rm replicate.c
	./string-to-char-array < attack.c > attack-array.h
	cat attack-array.h >> replicate.c
	cat trojan-cc2.c >> replicate.c
	$(CC) replicate.c -o replicate
	./replicate ../step2/identity-cc.c -o cc-attacked
	./cc-attacked ../step2/identity-cc.c -o cc-attacked2
	diff cc-attacked cc-attacked2
	./cc-attacked2 ../step2/login.c -o login-attacked
	./login-attacked

string-to-char-array: string-to-char-array.c
	$(CC) $(CFLAGS) string-to-char-array.c -o string-to-char-array

clean:
	rm -f  cc-attacked login-attacked out login identity-cc trojan-cc2 *.o a.out  *~ cc-attacked.c login-attacked.c out.c temp-out.c temp-out.c
