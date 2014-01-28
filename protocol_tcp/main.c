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
// Begin MAJ MSC
#include <netinet/tcp.h> // For TCP_NODELAY
#include <stddef.h>      // For offsetof
#define  ERROR_AT_LINE error_at_line
// End MAJ MSC
#include	"type.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include "bqueue.h"
#include "vector.h"

#define MAX_NODES 	5
#define STRING_SIZE		1024
#define	DELIMITER ' '
#define ACK_SIZE 15

typedef struct node_info_t {
	typId self_id;
	char IP[16];
	int port;
} node_info;
typedef struct config_info_t {
	typId self_id;
	node_info nodes[MAX_NODES];
	int num_nodes;
	int self_sid;
	int successor_sid;
} config_info;
typedef struct message_info_t{
	bcast *message;
	int bits;
} msg_info;

typedef struct counters_t {
	long messages_delivered;
	long messages_bytes_delivered;
	long messages_delivered_mesure;
	long messages_bytes_delivered_mesure;
	double delivered_latency;
}counters;

void validate_input_parameters(int argc, char **argv);
void parse_config_file(typId self_id, const char * config_file_name, int num_nodes);
void get_ip_port(const char* config_file1, config_info * config);
void create_ring();
void create_tcp_server_socket_for_predecessor();
void create_tcp_client_socket_for_successor();
void initialize();
void start_leader();
void stop_leader();
int get_substr(int start_index, int end_index, char * source, char * result_to_return);
void create_tcp_client_socket_for_successor();
void handle_msg(config_info *config, bcast *message);
void handle_ack(config_info * config, ack *recv_msg);
typId get_predecessor(typId p_id);
typId get_pre_predecessor(typId p_id);
void create_tcp_client_socket_for_successor();
int create_tcp_client_socket(const char *ip, int port);
void update_vector(bcast *message);
void move_msg_in_vector_ack(config_info *config, ack *acknowledge);
void save_msg_in_vector_ack( bcast *message);

void *process_msg (void * arg);
void *send_msg (void * arg);
void *deliver_msg (void * arg);
void broadcast_msg(int dest, bcast *message);
bcast *prepare_msg(typId id,int sequence_number,int acknowledge_number, int totalSize);
void save_msg_in_vector_No_ack( bcast *message);
void broadcast_ack(int successor_sid,ack *new_ack);
ack * prepare_ack(typId id,int sequence_number,int recv_number);
double measure_time_difference(struct timeval now, struct timeval past);
double get_mili_seconds();
double calculate_throughput(int bytes, double elapsed_time);
config_info *config_sample;
char *config_file = "config_file.dat";
typId self_id;
int num_nodes, time_out;
int interval_preparation, interval_warmingUp, interval_mesure, interval_stop, total_size;
int conf_bits = 0;
pthread_t th1, th2, th3;
void *ret;
static sem_t my_sem;

int seq_num, ack_num;

int mesurement_started = 0;
struct timeval timeBegin, timeEnd;
counters counters_sample;
int len_head_bcast;

vector_ *vector_no_ack[MAX_NODES];
vector_ *vector_ack;

int highestDelivered = 0;
trBqueue *deliveryQueue;

int main(int argc, char ** argv) {

	validate_input_parameters(argc, argv);
	parse_config_file(self_id, config_file, num_nodes);
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
		//send NOK to successor to start the test
		printf("stoping tests\n");
		stop_leader();
	}
	(void)pthread_join (th1, &ret);
	if(config_sample->self_id == 0){
		printf("==============================\n");
		printf("TOTAL_TIME_MESUREMENT(ms): %f\n", measure_time_difference(timeEnd, timeBegin));
		printf("MESSAGES_DELIVERED: %ld\nBYTES_DELIVERED: %ld\nDELIVERED_LATENCY(ms): %f\n", counters_sample.messages_delivered_mesure, counters_sample.messages_bytes_delivered_mesure, counters_sample.delivered_latency/counters_sample.messages_delivered_mesure);
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

void parse_config_file(typId self_id, const char * config_file_name, int num_nodes) {
	config_sample = (config_info*) malloc(sizeof(config_info));
	config_sample->self_id = self_id;
	config_sample->num_nodes = num_nodes;

	get_ip_port(config_file_name, config_sample);
	printf("creating ring ... \n");
	create_ring();// create a server and then create a client
	//printf("create ring ... OK\n");
}


void get_ip_port(const char* config_file, config_info * config) {

	FILE * fp;
	char line[STRING_SIZE];
	char word[STRING_SIZE];
	int last_index = -1, node_counter = 0;

	fp = fopen(config_file, "r");
	assert(fp != NULL);
	while (!feof(fp)) {
		if (fgets(line, STRING_SIZE, fp) != 0) {
			if(line[0] == '#'){
				continue;
			}

			last_index = get_substr(0, strlen(line) - 1, line, word);
			config->nodes[node_counter].self_id = (typId)atoi(word);
			last_index = get_substr(last_index + 1, strlen(line) - 1, line, word);
			memcpy(config->nodes[node_counter].IP, word, strlen(word));
			last_index = get_substr(last_index + 1, strlen(line) - 1, line, word);
			config->nodes[node_counter].port = atoi(word);

			//printf("node_counter = %d, port = %d\n",node_counter,config->nodes[node_counter].port);
			node_counter++;
		}
	}
	fclose(fp);
}

int get_substr(int start_index, int end_index, char * source, char * result_to_return) {
	int i = 0, j = 0;

	memset(result_to_return, '\0', STRING_SIZE);

	for (i = start_index; i < end_index; i++) {
		if (source[i] == DELIMITER)
			break;
		result_to_return[j] = source[i];
		j++;
	}
	return i;
}
void create_ring(){
	if(config_sample->self_id != 0){
		create_tcp_server_socket_for_predecessor();
		create_tcp_client_socket_for_successor();
	}else{
		create_tcp_client_socket_for_successor();
		create_tcp_server_socket_for_predecessor();
	}
}
// Begin MAJ MSC
void setTCP_NODELAY(int fd) {
  int status = 1;
  // We set TCP_NODELAY flag so that packets sent on this TCP connection
  // will not be delayed by the system layer
  assert(setsockopt(fd,IPPROTO_TCP, TCP_NODELAY, &status,sizeof(status)) >= 0);
}
// End MAJ MSC
void create_tcp_server_socket_for_predecessor() {
	unsigned int client_addr_len;
	int sid, e = -1, activate = 1;
	struct sockaddr_in server_addr, client_addr;
	client_addr_len = sizeof(client_addr);

	sid = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(sid, SOL_SOCKET, SO_REUSEADDR, &activate, sizeof(int));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(config_sample->nodes[config_sample->self_id].IP);
	server_addr.sin_port = htons(config_sample->nodes[config_sample->self_id].port);
	e = bind(sid, (struct sockaddr*) &server_addr, sizeof(server_addr));
	assert(e >= 0);

	e = listen(sid, 1);
	assert(e >= 0);

	printf("\nwaiting for connection request...\n");

	config_sample->self_sid = accept(sid, (struct sockaddr*) &client_addr, &client_addr_len);
	setsockopt(config_sample->self_sid, SOL_SOCKET, SO_REUSEADDR, &activate, sizeof(int));

	while (config_sample->self_sid < 0) {
		config_sample->self_sid = accept(sid, (struct sockaddr*) &client_addr, &client_addr_len);
		// Begin MAJ MSC
		setTCP_NODELAY(config_sample->self_sid);
		// End MAJ MSC
	}
	//int sockbufsize = 0; int size1 = sizeof(int);
	//getsockopt(config_sample->self_sid, SOL_SOCKET, SO_RCVBUF,
	//(char *)&sockbufsize, &size1);
	//printf("buffer size socket=%d\n", sockbufsize);
}
void create_tcp_client_socket_for_successor(){
	typId id_succ;
	if(config_sample->self_id != config_sample->num_nodes - 1){
		id_succ = config_sample->self_id +1;
	}else{
		id_succ = 0;
	}
	config_sample->successor_sid = create_tcp_client_socket(config_sample->nodes[id_succ].IP,
																		config_sample->nodes[id_succ].port);
}
int create_tcp_client_socket(const char *ip, int port) {
	struct sockaddr_in server_addr;
	int activate = 1;
	int e = -1;
	int sid;
	memset(&server_addr, '\0', sizeof(struct sockaddr_in));

	sid = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(sid, SOL_SOCKET, SO_REUSEADDR, &activate, sizeof(int));
	//setsock_nonblock(sid);

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);

	server_addr.sin_port = htons(port);

	while (e < 0) {
		e = connect(sid, (struct sockaddr*) &server_addr, sizeof(struct sockaddr));
		if (e >= 0)
			break;

		close(sid);
		sleep(1);
		sid = socket(AF_INET, SOCK_STREAM, 0);
		//setsock_nonblock(sid);
	}
	// Begin MAJ MSC
	setTCP_NODELAY(sid);
	// End MAJ MSC
	//setsock_nonblock(sid);
	return sid;
}
void initialize(){

	seq_num = 0;
	ack_num = 0;
	//rev_num = 0;

	conf_bits = ((1 << config_sample->num_nodes) - 1) & ~(1 << config_sample->self_id);

	// Begin MAJ MSC
	len_head_bcast = offsetof(bcast, startMiliSeconds);
	// End MAJ MSC

	int i = 0;
	for(i = 0;i < config_sample->num_nodes;i++){
		vector_no_ack[i] = createVector(5000);
	}
	vector_ack = createVector(5000);

	counters_sample.delivered_latency = 0;
	counters_sample.messages_bytes_delivered = 0;
	counters_sample.messages_delivered = 0;
	counters_sample.messages_bytes_delivered_mesure = 0;
	counters_sample.messages_delivered_mesure = 0;
	deliveryQueue = newBqueue();
}
void *process_msg (void * arg){
	int status;
	int  sign = 1;
	int started = 0;

	while(sign){
		//if(config_sample->self_id != 0){
		int len;
		msg *m;
		status = recv(config_sample->self_sid, &len, sizeof(len), MSG_WAITALL);
		assert(status == sizeof(len));
		m = malloc(len);
		assert(m != NULL);
		m->a.len = len;
		status = recv(config_sample->self_sid, &(m->a.type), len-sizeof(len), MSG_WAITALL);
		assert(status == len-sizeof(len));
		switch(m->a.type){
			case'y':{
				if(!started){
					printf("leader is starting the tests...\n");
					if(config_sample->self_id != 0){
						start_leader();
					}
					sem_post(&my_sem);
				}
				started = 1;
				// Begin MAJ MSC
				free(m);
				// End MAJ MSC
				break;
			}
			case'b':
			  handle_msg(config_sample, (bcast *)m);
				break;
			case'a':
			  handle_ack(config_sample, (ack *)m);
			  // Begin MAJ MSC
			  free(m);
			  // End MAJ MSC
				break;
			case'n':
				if(config_sample->self_id != 0){
					stop_leader();
				}
				// Begin MAJ MSC
				free(m);
				// End MAJ MSC
				pthread_exit (0);
				break;
			default: {
			  printf("Unexpected message type received : %d/\"%c\"\n", m->a.type, m->a.type);
			}
		}
	}
	pthread_exit (0);
}
void *send_msg (void * arg){
	sem_wait (&my_sem);
	while(1){
		seq_num++;
		bcast *msg_sample = prepare_msg(config_sample->self_id, seq_num, ack_num, total_size);

		#ifdef TRACES
		printf("Broadcast [id=%d seq=%d ack=%d] ->\n",config_sample->self_id,seq_num,ack_num);
		#endif
		broadcast_msg(config_sample->successor_sid, msg_sample);
		if(config_sample->self_id == 0){
			save_msg_in_vector_ack(msg_sample);
			update_vector(msg_sample);
			ack_num++;
			ack *ack_new = prepare_ack(msg_sample->id_process, msg_sample->seq, ack_num);
			broadcast_ack(config_sample->successor_sid, ack_new);
			free(ack_new);
			#ifdef TRACES
				printf("Ack  [id=%d seq=%d rev=%d] ->\n",ack_new->id_process, ack_new->seq, ack_new->revNum);
			#endif

		} else {
			save_msg_in_vector_No_ack(msg_sample);
		}
		usleep(time_out);
	}
	pthread_exit (0);
}
void *deliver_msg (void * arg){
	int c_t = 0;
	while(1){
			c_t++;
			msg_info *m = (msg_info *)bqueueDequeue(deliveryQueue);
		#ifdef TRACES
			printf("Delivery <message N°%d from processus %d>%d\n", m->message->seq, m->message->id_process,c_t);
		#endif
		if(mesurement_started){

		  // Begin MAJ MSC
		  if (m->message->id_process == config_sample->self_id) {
			// This process is the sender of this message to deliver.
			// ==> We can compare our clock and the clock stored in the message
			double lat = get_mili_seconds() - m->message->startMiliSeconds;
			counters_sample.delivered_latency += lat;
			counters_sample.messages_delivered_mesure++;
			counters_sample.messages_bytes_delivered_mesure += m->message->len - len_head_bcast;
		  }
		  // End MAJ MSC
		  counters_sample.messages_delivered++;
		  counters_sample.messages_bytes_delivered += m->message->len - len_head_bcast;
		}
			free(m->message);
			m->message = NULL;
			free(m);
			m = NULL;
	}
	pthread_exit (0);
}
void broadcast_msg(int dest, bcast *message){
	int status = send(dest, message, message->len, MSG_WAITALL);
	assert(status == message->len);
}
bcast *prepare_msg(typId id,int sequence_number,int acknowledge_number,int totalSize){
	bcast *new_msg = malloc(totalSize);
	new_msg->len = totalSize;
	new_msg->type = 'b';
	new_msg->id_process = id;
	new_msg->ack = acknowledge_number;
	new_msg->seq = sequence_number;
	// Begin MAJ MSC
	memset(new_msg->payload,0,totalSize-offsetof(bcast,payload));
	new_msg->startMiliSeconds = get_mili_seconds();
	// End MAJ MSC
	return new_msg;
}
void broadcast_ack(int successor_sid,ack *new_ack){

	int status = send(successor_sid, new_ack, new_ack->len, MSG_WAITALL);
	assert(status == new_ack->len);

}
ack * prepare_ack(typId id,int sequence_number,int recv_number){
	ack *new_ack =  malloc(ACK_SIZE);
	new_ack->len = ACK_SIZE;
	new_ack->type = 'a';
	new_ack->id_process = id;
	new_ack->seq = sequence_number;
	new_ack->revNum = recv_number;
	return new_ack;
}
void transfer_msg(config_info *config, bcast *message){

	int status = send(config->successor_sid, message, message->len, MSG_WAITALL);
	assert(status == message->len);
}
void transfer_ack(config_info *config, ack *acknowledge){

	int status = send(config->successor_sid, acknowledge, acknowledge->len, MSG_WAITALL);
	assert(status == acknowledge->len);
}
void start_leader(){
	ok *ok_var = malloc(sizeof(ok)) ;
	ok_var->len = sizeof(ok);
	ok_var->type = 'y';
	int status = send(config_sample->successor_sid, ok_var, ok_var->len, MSG_WAITALL);
	assert(status == ok_var->len);
	free(ok_var);
	ok_var = NULL;
}
void stop_leader(){
	ok *ok_var = malloc(sizeof(ok)) ;
	ok_var->len = sizeof(ok);
	ok_var->type = 'n';
	int status = send(config_sample->successor_sid, ok_var, ok_var->len, MSG_WAITALL);
	assert(status == ok_var->len);
	free(ok_var);
	ok_var = NULL;
}
void handle_msg(config_info * config, bcast *recv_msg){

	int predecessor;
	int self_id = config->self_id;
	#ifdef TRACES
	printf("<- recv [id=%d seq=%d ack=%d]\n", recv_msg->id_process, recv_msg->seq, recv_msg->ack);
	#endif

	predecessor = get_predecessor(recv_msg->id_process);
	if (config->self_id != predecessor) {

		#ifdef TRACES
		printf("Broadcast  [id=%d seq=%d ack=%d] ->\n", recv_msg->id_process, recv_msg->seq, recv_msg->ack);
		#endif

		transfer_msg(config,recv_msg);
	}
	update_vector(recv_msg);
	if(self_id == 0){
		save_msg_in_vector_ack(recv_msg);
		//rev_num++;
		ack_num++;// = rev_num;
		ack *ack_new = prepare_ack(recv_msg->id_process, recv_msg->seq, ack_num);
		broadcast_ack(config->successor_sid, ack_new);
		free(ack_new);
		#ifdef TRACES
		printf("Ack  [id=%d seq=%d rev=%d] ->\n",ack_new->id_process, ack_new->seq, ack_new->revNum);
		#endif
	}else{
		save_msg_in_vector_No_ack(recv_msg);
	}
}
void handle_ack(config_info * config, ack *recv_ack){

	typId self_id = config->self_id;
	#ifdef TRACES
	printf("<- recv_ack [id=%d seq=%d rev=%d] ->\n",recv_ack->id_process, recv_ack->seq, recv_ack->revNum);
	#endif
	if(self_id != 0){
		// Only a non-leader has to do something
		move_msg_in_vector_ack(config, recv_ack);
		// Begin MAJ MSC
		ack_num = recv_ack->revNum;
		// End MAJ MSC
		if ((config_sample->self_id + 1) % config_sample->num_nodes != 0) {
			// My successor is not the leader ==> I must forward the message
			transfer_ack(config, recv_ack);

			#ifdef TRACES
				printf("Ack_t  [id=%d seq=%d rev=%d] ->\n",recv_ack->id_process, recv_ack->seq, recv_ack->revNum);
			#endif
		}
	}
}
typId get_predecessor(typId p_id) {

	int predecessor;
	if (p_id == 0)
		predecessor = config_sample->num_nodes - 1;
	else
		predecessor = p_id - 1;
	return predecessor;
}
typId get_pre_predecessor(typId p_id) {
	int pre_predecessor;
	int predecessor = get_predecessor(p_id);

	if (predecessor == 0)
		pre_predecessor = config_sample->num_nodes - 1;
	else
		pre_predecessor = predecessor - 1;
	return pre_predecessor;
}
void save_msg_in_vector_ack( bcast *message){
	msg_info *msg_inf = malloc(sizeof(msg_info));
	msg_inf->message = message;
	msg_inf->bits = conf_bits;

	int status;
	status = addElt(msg_inf, vector_ack);
	assert(status == 1);

}
void move_msg_in_vector_ack(config_info *config, ack *acknowledge){
	int status;
	typId message_id = acknowledge->id_process;
	msg_info *msg_inf = (msg_info *)malloc(sizeof(msg_info));
	msg_inf->message = removeFirst(vector_no_ack[message_id]);
	msg_inf->bits = conf_bits;


	status = addElt(msg_inf, vector_ack);
	assert(status == 1);
}
void save_msg_in_vector_No_ack( bcast *message){
	typId message_id = message->id_process;

	int status;
	status = addElt(message, vector_no_ack[message_id]);
	assert(status == 1);
}
void update_vector(bcast *message){
	typId id = message->id_process;
	int ack_tmp = message->ack;
	int i;
	// Begin MAJ MSC
	for (i = 0 ; i < numberOfElt(vector_ack) ; i++) {
		msg_info *m = (msg_info *)   (elementAt(i, vector_ack));
		if (i + highestDelivered <= ack_tmp) {
		  m->bits &= ~(1 << id);
		}
	}
	// End MAJ MSC
	while (numberOfElt(vector_ack)>0 && (((msg_info *)(elementAt(0, vector_ack)))->bits == 0)) {
	  msg_info *m = removeFirst(vector_ack);
	  bqueueEnqueue(deliveryQueue, m);
	  highestDelivered++;
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
