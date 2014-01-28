#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<assert.h>
#include	<pthread.h>
#include 	<semaphore.h>
#include 	<sys/types.h>
#include 	<sys/socket.h>
#include 	<sys/time.h>
#include 	<stdint.h>
#include 	<unistd.h>
#include 	<stddef.h>      // For offsetof
#define  ERROR_AT_LINE error_at_line

#include	"type.h"

#include 	<netinet/in.h>
#include 	<arpa/inet.h>

#include 	"bqueue.h"

#include 	"vector.h"

#define MAX_NODES 	5
#define STRING_SIZE		1024
#define MIN_PORT 1024    /* minimum port allowed for multicast */
#define MAX_PORT 65535   /* maximum port allowed for multicast*/
#define MAX_LEN  10000    /* maximum string size to multicast */
#define ACK_SIZE 11
typedef struct config_info_t {
	typId self_id;
	int num_nodes;
	int self_sid;
} config_info;

typedef struct message_info_t{
	int bits;
	mcast *message;
} __attribute__((packed))msg_info;

typedef struct counters_t {
	long messages_delivered;
	long messages_bytes_delivered;
	long messages_delivered_mesure;
	long messages_bytes_delivered_mesure;
	double delivered_latency;
}counters;

void validate_input_parameters(int argc, char **argv);
void parse_config_file(typId self_id, int num_nodes);
void create_multicast_socket(config_info *config);
void initialize();
void start_leader();
void stop_leader();

void *process_msg (void * arg);
void *send_msg (void * arg);
void *deliver_msg (void * arg);

mcast *prepare_msg(typId id,int sequence_number,int acknowledge_number);
void multicast_msg( mcast *message);
void save_msg_in_vector_ack( mcast *message);
void update_vector(mcast *message);
void update_vector_1(mcast *message);
ack * prepare_ack(typId id_process,int sequence_number,int recv_number);
void multicast_ack(ack *new_ack);
void save_msg_in_vector_No_ack( mcast *message);
void move_msg_in_vector_ack(config_info *config, ack *acknowledge);
void handle_msg(config_info * config, mcast *recv_msg);
void handle_ack(config_info * config, ack *recv_ack);
void update_prepare_bits(int id);
double measure_time_difference(struct timeval now, struct timeval past);
double get_mili_seconds();
double calculate_throughput(int bytes, double elapsed_time);

config_info *config_sample;
typId self_id;
int num_nodes, time_out;
int interval_preparation, interval_warmingUp, interval_mesure, interval_stop, total_size, ack_size = ACK_SIZE;
int conf_bits, prep_bits;

pthread_t th1, th2, th3;
void *ret;
static sem_t my_sem;

int mesurement_started = 0;
struct timeval timeBegin, timeEnd;
int seq_num, ack_num;
vector_ *vector_no_ack[MAX_NODES];
vector_ *vector_ack;
int len_head_mcast;

counters counters_sample;
trBqueue *deliveryQueue;
int test_started = 0;
int highestDelivered = 0;

char* mc_addr_str = "239.255.0.0";
int mc_port = 4321;
struct sockaddr_in mc_addr; /* socket address structure */

int main(int argc, char ** argv) {

	validate_input_parameters(argc, argv);
	parse_config_file(self_id, num_nodes);
	create_multicast_socket(config_sample);
	printf("create socket for multicasting ... OK\n");
	initialize();
	sem_init(&my_sem, 0, 0);
	if (pthread_create (&th1, NULL, process_msg, "1") < 0) {
		fprintf (stderr, "pthread_create error for thread 1\n");
		exit (1);
	}
	if (pthread_create (&th2, NULL, send_msg, "2") < 0) {
		fprintf (stderr, "pthread_create error for thread 2\n");
		exit (1);
	}
	if (pthread_create (&th3, NULL, deliver_msg, "3") < 0) {
		fprintf (stderr, "pthread_create error for thread 3\n");
		exit (1);
	}

	if(config_sample->self_id == 0){
		printf("sleeping... %d s\n",interval_preparation);
		sleep(interval_preparation);
		start_leader();
		printf("warming up... %d s\n",interval_warmingUp);
		sleep(interval_warmingUp);
		if (gettimeofday(&timeBegin, NULL ) < 0){
			ERROR_AT_LINE(EXIT_FAILURE, errno, __FILE__, __LINE__, "gettimeofday");
		}
		mesurement_started = 1;
		printf("waiting... %d s\n",interval_mesure);
		sleep(interval_mesure);
		if (gettimeofday(&timeEnd, NULL ) < 0){
			ERROR_AT_LINE(EXIT_FAILURE, errno, __FILE__, __LINE__, "gettimeofday");
		}
		mesurement_started = 0;
		sleep(interval_stop);
		printf("stoping tests\n");
		stop_leader();
	}
	(void)pthread_join (th1, &ret);
	//(void)pthread_join (th2, &ret);
	//(void)pthread_join (th3, &ret);
	if(config_sample->self_id == 0){
			printf("==============================\n");
			printf("TOTAL_TIME_MESUREMENT(ms): %f\n", measure_time_difference(timeEnd, timeBegin));
			printf("MESSAGES_DELIVERED: %ld\nBYTES_DELIVERED: %ld\nDELIVERED_LATENCY(ms): %f\n", counters_sample.messages_delivered_mesure,
					counters_sample.messages_bytes_delivered_mesure, counters_sample.delivered_latency/counters_sample.messages_delivered_mesure);
			printf("THROUGHPUT: %f(Mbps)\n",calculate_throughput(counters_sample.messages_bytes_delivered_mesure, measure_time_difference(timeEnd, timeBegin)));
	}
	return 0;
}
void validate_input_parameters(int argc, char **argv) {
	if (argc != 9) {
		printf("Incompatible call to this function. Try Again.!\n");
		printf("<1. Process ID>\n"
			"<2. Number of Process>\n"
			"<3. time-out(MicroSeconds)>\n"
			"<4. interval of preparation>\n"
			"<5. interval to warm up>\n"
			"<6. interval to mesure the performance>\n"
			"<7. interval to stop broadcasting the packets\n"
		    "<8. total_size of broadcast messages\n");
		exit(1);
	} else {
		self_id = (typId)atoi(argv[1]);
		num_nodes = atoi(argv[2]);
		time_out = atoi(argv[3]);
		interval_preparation = atoi(argv[4]);
		interval_warmingUp = atoi(argv[5]);
		interval_mesure = atoi(argv[6]);
		interval_stop = atoi(argv[7]);
		total_size = atoi(argv[8]);
	}
}


void parse_config_file(typId self_id, int num_nodes) {
	config_sample = (config_info*) malloc(sizeof(config_info));
	config_sample->self_id = self_id;
	config_sample->num_nodes = num_nodes;

}

void create_multicast_socket(config_info * config){
	unsigned char mc_ttl=1;     /* time to live (hop count) */
	unsigned char loopch=1;     /* we want to receive our own datagram */
	int flag_on = 1;              /* socket option flag */
	struct ip_mreq mc_req;        /* multicast request structure */


	/* validate the port range */
	if ((mc_port < MIN_PORT) || (mc_port > MAX_PORT)) {
	fprintf(stderr, "Invalid port number argument %d.\n",
			mc_port);
	fprintf(stderr, "Valid range is between %d and %d.\n",
			MIN_PORT, MAX_PORT);
	exit(1);
	}

	/* create a socket for sending to the multicast address */
	if ((config->self_sid = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
	perror("socket() failed");
	exit(1);
	}

	/* set the TTL (time to live/hop count) for the send */
	if ((setsockopt(config->self_sid, IPPROTO_IP, IP_MULTICAST_TTL,
	   (void*) &mc_ttl, sizeof(mc_ttl))) < 0) {
		perror("setsockopt() failed");
		exit(1);
	}

	/*
	   * Enable loopback so we do receive our own datagrams.
	   */
	if (setsockopt(config->self_sid, IPPROTO_IP, IP_MULTICAST_LOOP,
				   (char *)&loopch, sizeof(loopch)) < 0) {
	  perror("setting IP_MULTICAST_LOOP:");
	  exit(1);
	}

	/* construct a multicast address structure */
	memset(&mc_addr, 0, sizeof(mc_addr));
	mc_addr.sin_family      = AF_INET;
	mc_addr.sin_addr.s_addr = inet_addr(mc_addr_str);
	mc_addr.sin_port        = htons(mc_port);

	/* set reuse port to on to allow multiple binds per host */
	if ((setsockopt(config->self_sid, SOL_SOCKET, SO_REUSEADDR, &flag_on,
	   sizeof(flag_on))) < 0) {
		perror("setsockopt() failed");
		exit(1);
	}

	/* bind to multicast address to socket */
	if ((bind(config->self_sid, (struct sockaddr *) &mc_addr,
	   sizeof(mc_addr))) < 0) {
		perror("bind() failed");
		exit(1);
	}

	/* construct an IGMP join request structure */
	mc_req.imr_multiaddr.s_addr = inet_addr(mc_addr_str);
	mc_req.imr_interface.s_addr = htonl(INADDR_ANY);

	/* send an ADD MEMBERSHIP message via setsockopt */
	if ((setsockopt(config->self_sid, IPPROTO_IP, IP_ADD_MEMBERSHIP,
	   (void*) &mc_req, sizeof(mc_req))) < 0) {
		perror("setsockopt() failed");
		exit(1);
	}
	/*int sockbufsize = 0; int size1 = sizeof(int);
	getsockopt(config->self_sid, SOL_SOCKET, SO_RCVBUF,
		(char *)&sockbufsize, &size1);
	printf("RECV buffer size socket=%d\n", sockbufsize);
	getsockopt(config->self_sid, SOL_SOCKET, SO_SNDBUF,
			(char *)&sockbufsize, &size1);
	printf("SEND buffer size socket=%d\n", sockbufsize);
	*/

}
void initialize(){

	seq_num = 0;
	ack_num = 0;

	conf_bits = (1 << config_sample->num_nodes) - 1;
	prep_bits = ((1 << config_sample->num_nodes) - 1 )& ~(1 << config_sample->self_id);

	len_head_mcast = offsetof(mcast, startMiliSeconds);
	int i = 0;
	for(i = 0;i < config_sample->num_nodes;i++){
		vector_no_ack[i] = createVector(5000);
	}
	vector_ack = createVector(5000);

	counters_sample.delivered_latency = 0.0;
	counters_sample.messages_bytes_delivered = 0;
	counters_sample.messages_delivered = 0;
	counters_sample.messages_bytes_delivered_mesure = 0;
	counters_sample.messages_delivered_mesure = 0;
	deliveryQueue = newBqueue();
}
void *process_msg (void * arg){

	while(1){
		char *recv_buf = malloc(total_size);     /* buffer to receive string */
		int recv_len;                 /* length of string received */
		int sign = 0;

		/* block waiting to receive a packet */
		if ((recv_len = recvfrom(config_sample->self_sid, recv_buf, total_size, 0,
			NULL, NULL)) < 0) {
		  perror("recvfrom() failed");
		  exit(1);
		}
		if(recv_len > MAX_LEN){
			perror("Message is so big\n");
		}
		//printf("get%c len=%d\n",recv_buf[0], recv_len);

		switch(recv_buf[0]){
			case'm':{
				handle_msg(config_sample, (mcast *)recv_buf);
				break;
			}
			case'a':{
				handle_ack(config_sample, (ack *)recv_buf);
				free(recv_buf);
				recv_buf = NULL;
				break;
			}
			case'y':{
				char str[2];
				if(config_sample->self_id != 0){
					if(!sign){
						str[0] = 'z';
						str[1] = '0' + config_sample->self_id;;
						if ((sendto(config_sample->self_sid, str, sizeof(str), 0,
								   (struct sockaddr *) &mc_addr,
									sizeof(mc_addr))) != 2) {
							perror("sendto() sent z message");
							exit(1);
						}
						sign = 1;
					}
				}else {
					if(prep_bits == 0){
						char c_t = 'c';
						if ((sendto(config_sample->self_sid, &c_t, sizeof(c_t), 0,
						   (struct sockaddr *) &mc_addr,sizeof(mc_addr))) != 1) {
								perror("sendto() sent incorrect c message");
								exit(1);
						}
						test_started = 1;
					}
				}
				free(recv_buf);
				recv_buf = NULL;
				break;
			}
			case'z':{
				if(config_sample->self_id == 0){
					update_prepare_bits(recv_buf[1] - '0');
				}
				free(recv_buf);
				recv_buf = NULL;
				break;
			}
			case'n':{
				free(recv_buf);
				recv_buf = NULL;
				pthread_exit (0);
				break;
			}
			case'c':{
				sem_post(&my_sem);
				free(recv_buf);
				recv_buf = NULL;
				break;
			}
			default:{
				printf("Unexpected message type received : %d\n", recv_buf[0]);
				free(recv_buf);
				recv_buf = NULL;
			}
		}
	}
	pthread_exit (0);
}
void *send_msg (void * arg){
	sem_wait (&my_sem);

	while(1){
		seq_num++;
		mcast *msg_sample = prepare_msg(config_sample->self_id, seq_num, ack_num);
		#ifdef TRACES
		printf("Multicast [id=%d seq=%d ack=%d] ->\n",config_sample->self_id,seq_num,ack_num);
		#endif
		multicast_msg(msg_sample);
		free(msg_sample);
		usleep(time_out);
	}
	pthread_exit (0);
}
void *deliver_msg (void * arg){
	int c_t = 0;

	while(1){
		msg_info *m = (msg_info *)bqueueDequeue(deliveryQueue);
		#ifdef TRACES
		printf("Delivery <message N°%d from processus %d>%d\n", m->message->seq, m->message->id_process,c_t);
		#endif
		c_t++;
		if(mesurement_started){

			// Begin MAJ MSC
			if (m->message->id_process == config_sample->self_id) {
				// This process is the sender of this message to deliver.
				// ==> We can compare our clock and the clock stored in the message
				double lat = get_mili_seconds() - m->message->startMiliSeconds;
				counters_sample.delivered_latency += lat;
				counters_sample.messages_delivered_mesure++;
				counters_sample.messages_bytes_delivered_mesure += total_size - len_head_mcast;
			}
			// End MAJ MSC
			counters_sample.messages_delivered++;
			counters_sample.messages_bytes_delivered += total_size - len_head_mcast;
		}
		free(m->message);
		m->message = NULL;
		free(m);
		m = NULL;
	}

	pthread_exit (0);
}
void update_prepare_bits(int id){
	prep_bits &= ~(1 << id);
}
mcast *prepare_msg(typId id,int sequence_number,int acknowledge_number){

	mcast *new_msg = malloc(total_size);
	//printf("sizeof(new_msg) = %d\n",sizeof(new_msg));
	new_msg->type = 'm';
	new_msg->id_process = id;
	new_msg->ack = acknowledge_number;
	new_msg->seq = sequence_number;
	// Begin MAJ MSC
	new_msg->startMiliSeconds = get_mili_seconds();
	memset(new_msg->payload,'0'+9,total_size-offsetof(mcast,payload));
	// End MAJ MSC
	return new_msg;
}
void multicast_msg(mcast *message){
	if ((sendto(config_sample->self_sid, (char *)message, total_size, 0,
	   (struct sockaddr *) &mc_addr, sizeof(mc_addr))) != total_size) {
		perror("sendto() multicast msg\n");
		exit(1);
	}
}
void save_msg_in_vector_ack( mcast *message){
	msg_info *msg_inf = malloc(sizeof(msg_info));
	//printf("mcast before save in vector msg id = %d, msg seq = %d\n",message->id_process,message->seq);
	msg_inf->message = message;
	msg_inf->bits = conf_bits;
	int status;
	status = addElt(msg_inf, vector_ack);
	/*
	int i;
	printf("print all the Elt in the vector\n");
	for(i = 0; i < numberOfElt(vector_ack); i++){
		printf("***N°%d id=%d seq=%d\n",i,( (msg_info *)elementAt(i, vector_ack) )->message->id_process,
				( (msg_info *)   (elementAt(i, vector_ack)) )->message->seq);
	}
	printf("mcast saved in vector msg id = %d, msg seq = %d\n",msg_inf->message->id_process,msg_inf->message->seq);
	*/
	assert(status == 1);
}
void update_vector(mcast *message){
	//printf("up \n");
	typId id = message->id_process;
	int ack_tmp = message->ack;
	int i;
	// Begin MAJ MSC
	for (i = 0 ; i < numberOfElt(vector_ack) ; i++) {
		msg_info *m = (msg_info *) elementAt(i, vector_ack);
		//printf("manip (%d,%d) i =%d\n",( (msg_info *)elementAt(i, vector_ack) )->message->id_process,
			//	( (msg_info *)   (elementAt(i, vector_ack)) )->message->seq, i);

		//printf("i = %d | highest = %d | ack_tmp = %d\n", i, highestDelivered, ack_tmp);
		if (i + highestDelivered <= ack_tmp) {
		  m->bits &= ~(1 << id);
		  //printf("=== apres %d, sur %d/%d, m->bits = %04x\n", id, m->message->id_process, m->message->seq, m->bits);
		}
	}
	// End MAJ MSC
	while (numberOfElt(vector_ack)>0 && (((msg_info *)(elementAt(0, vector_ack)))->bits == 0)) {
	  msg_info *m = removeFirst(vector_ack);
	  //double lat = get_mili_seconds() - m->message->startMiliSeconds;
	  //printf("NbElet / Latency = %d / %g\n", numberOfElt(vector_ack), lat);
	  bqueueEnqueue(deliveryQueue, m);
	  highestDelivered++;
	}
}
void update_vector_1(mcast *message){
	//printf("up \n");
	typId id = message->id_process;
	int ack_tmp = message->ack;
	int i;
	// Begin MAJ MSC
	for (i = 0 ; i < numberOfElt(vector_ack) ; i++) {
		msg_info *m = (msg_info *)   (elementAt(i, vector_ack));
		//printf("manip (%d,%d)\n",( (msg_info *)   (elementAt(i, vector_ack)) )->message->id_process,( (msg_info *)   (elementAt(i, vector_ack)) )->message->seq);
		//printf("i = %d | highest = %d | ack_tmp = %d\n", i, highestDelivered, ack_tmp);
		if (i + highestDelivered < ack_tmp) {
		  m->bits &= ~(1 << id);
		  //printf("=== apres %d, sur %d/%d, m->bits = %04x\n", id, m->message->id_process, m->message->seq, m->bits);
		}
	}
	// End MAJ MSC
	//printf("nbElt = %d\n",vector_ack->nbElt);
	//printf("%d\n",((msg_info *)(elementAt(0, vector_ack)))->bits);
	while (numberOfElt(vector_ack)>0 && (((msg_info *)(elementAt(0, vector_ack)))->bits == 0)) {
	  msg_info *m = removeFirst(vector_ack);
	  //double lat = get_mili_seconds() - m->message->startMiliSeconds;
	  //printf("NbElet / Latency = %d / %g\n", numberOfElt(vector_ack), lat);
	  bqueueEnqueue(deliveryQueue, m);
	  highestDelivered++;
	}

}
ack *prepare_ack(typId id_process,int sequence_number,int recv_number){
	ack *new_ack = malloc(ack_size);
	new_ack->type = 'a';
	new_ack->id_process = id_process;
	new_ack->seq = sequence_number;
	new_ack->revNum = recv_number;
	return new_ack;
}
void multicast_ack(ack *new_ack){
	if ((sendto(config_sample->self_sid, new_ack, ack_size, 0,
		   (struct sockaddr *) &mc_addr, sizeof(mc_addr))) != ack_size) {

			perror("sendto() multicast ack\n");
			exit(1);
		}
}
void save_msg_in_vector_No_ack( mcast *message){
	typId message_id = message->id_process;
	int status;
	status = addElt(message, vector_no_ack[message_id]);
	assert(status == 1);
}
void handle_msg(config_info * config, mcast *recv_msg){
	typId self_id = config->self_id;

	#ifdef TRACES
	printf("<- recv [id=%d seq=%d ack=%d]\n", recv_msg->id_process, recv_msg->seq, recv_msg->ack);
	#endif

	if(self_id == 0){
		if(recv_msg->id_process != self_id){
			update_vector_1(recv_msg);
			save_msg_in_vector_ack(recv_msg);
		}else{
			save_msg_in_vector_ack(recv_msg);
			update_vector(recv_msg);
		}

		ack_num++;
		ack *ack_new = prepare_ack(recv_msg->id_process, recv_msg->seq, ack_num);
		multicast_ack(ack_new);
		#ifdef TRACES
		printf("Ack  [id=%d seq=%d rev=%d] ->\n",ack_new->id_process, ack_new->seq, ack_new->revNum);
		#endif
		free(ack_new);
	}else{
			save_msg_in_vector_No_ack(recv_msg);
			update_vector(recv_msg);
	}
}
void handle_ack(config_info * config, ack *recv_ack){

	typId self_id = config->self_id;
	#ifdef TRACES
	printf("<- recv_ack [id=%d seq=%d rev=%d]\n",recv_ack->id_process, recv_ack->seq, recv_ack->revNum);
	#endif

	if(self_id != 0){
		// Only a non-leader has to do something
		move_msg_in_vector_ack(config, recv_ack);
		// Begin MAJ MSC
		ack_num = recv_ack->revNum;
		// End MAJ MSC
	}
}
double get_mili_seconds() {
	struct timeval mtv;
	gettimeofday(&mtv, NULL);
	double time = 1000 * mtv.tv_sec + mtv.tv_usec / (double) 1000;
	return time;
}
double measure_time_difference(struct timeval now, struct timeval past) {
	struct timeval elapsed_interval;

	elapsed_interval.tv_sec = now.tv_sec - past.tv_sec;
	elapsed_interval.tv_usec = (now.tv_usec > past.tv_usec ? now.tv_usec - past.tv_usec : past.tv_usec - now.tv_usec);
	double msec_interval = 1000 * elapsed_interval.tv_sec + elapsed_interval.tv_usec / (double) 1000;

	return msec_interval;
}
double calculate_throughput(int bytes, double elapsed_time) {
	double thr = (bytes / (double) (1000 * 1000));
	thr = thr / (double) elapsed_time;
	thr = thr * 1000 * 8;
	return thr;
}
void start_leader(){
	char ok = 'y';
	while(!test_started){
		if ((sendto(config_sample->self_sid, &ok, sizeof(char), 0,
		   (struct sockaddr *) &mc_addr,
			sizeof(mc_addr))) != 1) {

			perror("sendto() start leader");
			exit(1);
		}
		usleep(10);
	}
	printf("start leader done\n");
}
void stop_leader(){
	char ok = 'n';
	if ((sendto(config_sample->self_sid, &ok, sizeof(char), 0,
	   (struct sockaddr *) &mc_addr,
		sizeof(mc_addr))) != 1) {

		perror("sendto() sent incorrect stop message");
		exit(1);
	}
	printf("test is stopping\n");
}
void move_msg_in_vector_ack(config_info *config, ack *acknowledge){
	int status;
	typId message_id = acknowledge->id_process;
	msg_info *msg_inf = (msg_info *)malloc(sizeof(msg_info));
	msg_inf->message = removeFirst(vector_no_ack[message_id]);
	msg_inf->bits = conf_bits;
	status = addElt(msg_inf, vector_ack);
	//printf("msg moved in vector id = %d, seq = %d\n",msg_inf->message->id_process, msg_inf->message->seq);
	assert(status == 1);
}
