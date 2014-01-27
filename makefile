all:
	gcc -g -I/mci/ei1114/wangyue/Software/TrainsProtocol/include -DTRACES -Wall -lm -lpthread -L/mci/ei1114/wangyue/Software/TrainsProtocol/src -ltrains main.c -o main
quick:
	gcc -g -I/mci/ei1114/wangyue/Software/TrainsProtocol/include -Wall -lm -lpthread -L/mci/ei1114/wangyue/Software/TrainsProtocol/src -ltrains main.c -o main
clean:
	rm -f core* *~

cleanall: clean
	rm -f main
