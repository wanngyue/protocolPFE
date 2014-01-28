all:
	gcc -I../include/ -c vector.c -o vector.o
	gcc -g -I. -I../include -DTRACES -Wall -lm -lpthread -L../src -ltrains mainUDP.c vector.o -o mainUDP
quick:
	gcc -I../include/ -c vector.c -o vector.o
	gcc -g -I. -I../include -Wall -lm -lpthread -L../src -ltrains mainUDP.c vector.o -o mainUDP
clean:
	rm -f core* *~

cleanall: clean
	rm -f main
