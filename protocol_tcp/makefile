all:
	gcc -g -I../include -DTRACES -Wall -lm -lpthread -L../src -ltrains main.c -o main
quick:
	gcc -g -I../include -Wall -lm -lpthread -L../src -ltrains main.c -o main
clean:
	rm -f core* *~

cleanall: clean
	rm -f main
