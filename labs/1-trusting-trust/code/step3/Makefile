# you should not need to modify this.
CC =gcc
CFLAGS = -Og -g -Wall -Wno-unused-variable

all: string-to-char-array clone

clone:
	../step1/string-to-char-array < attack.c > attack-array.h
	$(CC) trojan-cc2.c -o trojan-cc2
	./trojan-cc2 ../step2/identity-cc.c -o cc-attacked
	rm temp-out.c
	./cc-attacked ../step2/identity-cc.c -o cc-attacked2
	rm temp-out.c
	diff cc-attacked cc-attacked2
	./cc-attacked2 ../step2/login.c -o login-attacked
	./login-attacked
	rm temp-out.c

string-to-char-array: string-to-char-array.c
	$(CC) $(CFLAGS) string-to-char-array.c -o string-to-char-array

clean:
	rm -f  cc-attacked cc-attacked2 string-to-char-array login-attacked out login identity-cc trojan-cc2 *.o a.out  *~ cc-attacked.c login-attacked.c out.c temp-out.c temp-out.c
