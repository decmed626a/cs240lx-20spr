# you should not need to modify this.
CC =gcc
CFLAGS = -Og -g -Wall -Wno-unused-variable

all: string-to-char-array trojan-cc2 clone

clone:
	rm attack-array.h
	./string-to-char-array < attack.c > attack-array.h
	cat attack-array.h >> replicate.c
	cat trojan-cc2.c >> replicate.c
	$(CC) replicate.c -o replicate
	./replicate ../step2/identity-cc.c -o cc-attacked
	./cc-attacked ../step2/identity-cc.c -o cc-attacked2
	diff cc-attacked cc-attacked2

string-to-char-array: string-to-char-array.c
	$(CC) $(CFLAGS) string-to-char-array.c -o string-to-char-array

trojan-cc2: trojan-cc2.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f  cc-attacked login-attacked out login identity-cc trojan-cc1 *.o a.out  *~ cc-attacked.c login-attacked.c out.c temp-out.c temp-out.c
