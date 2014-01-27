all:
	gcc -g -I. -I../include -DTRACES -Wall -lm -lpthread -L../src -ltrains mainUDP.c -o mainUDP
quick:
	gcc -g -I. -I../include -Wall -lm -lpthread -L../src -ltrains mainUDP.c -o mainUDP
clean:
	rm -f core* *~

cleanall: clean
	rm -f main
