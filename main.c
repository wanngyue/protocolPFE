#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<assert.h>
#include	<pthread.h>
#include 	<semaphore.h>
#include 	<sys/types.h>
#include 	<sys/socket.h>
#include	"types.h"

#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_NODES 	5
#define STRING_SIZE		1024
#define	DELIMITER ' '

typedef struct node_info_t {
	int self_id;
	char IP[16];
	int port;
} node_info;
typedef struct config_info_t {
	int self_id;
	node_info nodes[MAX_NODES];
	int num_nodes;
	int self_sid;
	int successor_sid;
} config_info;

void validate_input_parameters(int argc, char **argv);
void parse_config_file(int self_id, const char * config_file_name, int num_nodes);
void get_ip_port(const char* config_file, config_info * config);
void create_ring();
void create_tcp_server_socket_for_predecessor();
void create_tcp_client_socket_for_successor();
void initialize();

void *process_msg (void * arg);
void *send_msg (void * arg);
void *deliver_msg (void * arg);
void broadcast_msg(config_info *config,int sequence_number,int acknowledge_number, msg *message);

config_info *config_sample;
char * config_file = "config_file.dat";

int self_id, num_nodes, time_out;
int interval_preparation, interval_warmingUp, interval_mesure, interval_stop;

pthread_t th1, th2, th3;
void *ret;
static sem_t my_sem;

int seq_num, ack_num, rev_num, conf_num;

int main(int argc, char ** argv) {

	validate_input_parameters(argc, argv);
	/*printf("self_id = %d;num_proposers = %d;time_out = %d;interval_preparation = %d;"
			"interval_warmingUp = %d;interval_mesure = %d;interval_stop = %d"
			,self_id,num_proposers,time_out,interval_preparation,interval_warmingUp,interval_mesure,interval_stop);
	*/
	parse_config_file(self_id, config_file, num_nodes);
	/*int i = 0;
	for(i;i < num_nodes;i++){

		printf("config_sample->nodes[%d].self_id = %d;config_sample->nodes[%d].IP = %s;config_sample->nodes[%d].port = %d"
				,i,config_sample->nodes[i].self_id,i,config_sample->nodes[i].IP,i,config_sample->nodes[i].port);

		printf("\n");
	}
	*/
	void initialize();

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
	(void)pthread_join (th1, &ret);
	(void)pthread_join (th2, &ret);
	(void)pthread_join (th3, &ret);

	if(config_sample->self_id == 0){
		sleep(interval_preparation);
		//send OK to successor to start the test
		start_leader();
		sleep(interval_warmingUp);
		//start timer
		sleep(interval_mesure);
		//stop timer
		sleep(interval_stop);
		//send NOK to successor to start the test
		stop_leader();
	}
}
void validate_input_parameters(int argc, char **argv) {
	if (argc != 8) {
		printf("Incompatible call to this function. Try Again.!\n");
		printf("<1. Process ID>\n"
			"<2. Number of Process>\n"
			"<3. time-out(MicroSeconds)>\n"
			"<4. interval of preparation>\n"
			"<5. interval to warm up>\n"
			"<6. interval to mesure the performance>\n"
			"<7. interval to stop broadcasting the packets\n");
		exit(1);
	} else {
		self_id = atoi(argv[1]);
		num_nodes = atoi(argv[2]);
		time_out = atoi(argv[3]);
		interval_preparation = atoi(argv[4]);
		interval_warmingUp = atoi(argv[5]);
		interval_mesure = atoi(argv[6]);
		interval_stop = atoi(argv[7]);
	}
}


void parse_config_file(int self_id, const char * config_file_name, int num_nodes) {
	config_sample = (config_info*) malloc(sizeof(config_info));
	config_sample->self_id = self_id;
	config_sample->num_nodes = num_nodes;

	get_ip_port(config_file_name, config_sample);

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
			config->nodes[node_counter].self_id = atoi(word);
			last_index = get_substr(last_index + 1, strlen(line) - 1, line, word);
			memcpy(config->nodes[node_counter].IP, word, strlen(word));
			last_index = get_substr(last_index + 1, strlen(line) - 1, line, word);
			config->nodes[node_counter].port = atoi(word);

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

	printf("\nwaiting for connection request from successor...\n");

	config_sample->self_sid = accept(sid, (struct sockaddr*) &client_addr, &client_addr_len);
	setsockopt(config_sample->self_sid, SOL_SOCKET, SO_REUSEADDR, &activate, sizeof(int));

	while (config_sample->self_sid < 0)
		config_sample->self_sid = accept(sid, (struct sockaddr*) &client_addr, &client_addr_len);
}
void create_tcp_client_socket_for_successor(){
	int id_succ;
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
	//setsock_nonblock(sid);
	return sid;
}
void initialize(){

	seq_num = 0;
	ack_num = 0;
	rev_num = 0;
	conf_num = 0;

	//TO DO

}

void *process_msg (void * arg){
	int status;
	char msg_type;

	while(1){
		//if(config_sample->self_id != 0){
		status = recv(config_sample->self_sid, &msg_type, sizeof(char), MSG_PEEK);
		assert(status == sizeof(char));
		switch(msg_type){
			case'y':
				printf("leader is starting the tests...\n");
				sem_post(&my_sem);
				if(config_sample->self_id != (config_sample->num_nodes - 1)){
					start_leader();
				}
				break;
			case'm':
				handle_msg();
				break;
			case'a':
				handle_ack();
				break;
			case's':
				printf("leader is stopping the tests...\n");
				if(config_sample->self_id != (config_sample->num_nodes - 1)){
					stop_leader();
				}
				exit(EXIT_SUCCESS);
			default: {
				printf("Unexpected message type received.%c\n", msg_type);
			}
		}
		//}
	}

	pthread_exit (0);
}
void *send_msg (void * arg){

	if(config_sample->self_id == 0){
		while(1){
			seq_num++;
			broadcast_msg(config_sample,seq_num, ack_num, msg_sample);
			save_msg_in_vector_No_ack();
			usleep(time_out);
		}
	}else{
		sem_wait (&my_sem);
		while(1){
			seq_num++;
			broadcast_msg(config_sample,seq_num, ack_num, msg_sample);
			save_msg_in_vector_No_ack();
			usleep(time_out);
		}
	}
	pthread_exit (0);
}
void *deliver_msg (void * arg){
//  int i;
//
//  for (i = 0 ; i < 5 ; i++) {
//    printf ("Thread %s: %d\n", (char*)arg, i);
//    sleep (1);
//  }

	pthread_exit (0);
}
void broadcast_msg(config_info *config,int sequence_number,int acknowledge_number, msg *message){
	msg *new_msg = (msg*) malloc(sizeof(msg));
	new_msg->id_process = config->self_id;
	new_msg->ack = ack_num;
	new_msg->seq = seq_num;
	new_msg->type = 'm';
	new_msg->len = sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int) + sizeof(double);

	int status = send(config->successor_sid, new_msg, sizeof(msg), MSG_WAITALL);
	assert(status == sizeof(msg));
}
void broadcast_ack(config_info *config,int sequence_number,int recv_number, int conf_number){
	ack *new_ack = (ack*) malloc(sizeof(ack));
	new_ack->id_process = config->self_id;
	new_ack->seq = sequence_number;
	new_ack->revNum = recv_number;
	new_ack->confNum = conf_number;
	new_ack->type = 'a';

	int status = send(config->successor_sid, new_ack, sizeof(ack), MSG_WAITALL);
	assert(status == sizeof(ack));
}
void start_leader(){
	ok *ok_var = (ok*)malloc(sizeof(ok)) ;
	ok_var->type = 'y';
	int status = send(config_sample->successor_sid, ok_var, sizeof(ok), MSG_WAITALL);
	assert(status == sizeof(ok));
}
void stop_leader(){
	ok *ok_var = (ok*)malloc(sizeof(ok)) ;
	ok_var->type = 'n';
	int status = send(config_sample->successor_sid, ok_var, sizeof(ok), MSG_WAITALL);
	assert(status == sizeof(ok));
}
void handle_msg(){

	int status, predecessor = 0, pre_predecessor = 0;

	msg recv_msg;
	int self_id = config_sample->self_id;

	status = recv(config_sample->self_sid, &recv_msg, sizeof(msg), MSG_WAITALL);
	assert(status == sizeof(msg));

	predecessor = get_predecessot(recv_msg.id_process);
	pre_predecessor = get_pre_predecessor(recv_msg.id_process);

	if(self_id == 0){
		save_msg_in_vector_ack();
		rev_num++;
		conf_num = confirm_msg();//confirm all the messages whose num < ack_num for id_process
		update_vector();

		broadcast_ack(config_sample, seq_num, rev_num, conf_num);//!!!!
	}else{
		save_msg_in_vector_No_ack();
	}
	if (config_sample->self_id != predecessor) {

	} else {


	}
}
void handle_ack(){





}
int get_predecessot(int p_id) {

	int predecessor;
	if (p_id == 0)
		predecessor = config_sample->num_nodes - 1;
	else
		predecessor = p_id - 1;
	return predecessor;
}
int get_pre_predecessor(int p_id) {
	int pre_predecessor;
	int predecessor = get_predecessot(p_id);

	if (predecessor == 0)
		pre_predecessor = config_sample->num_nodes - 1;
	else
		pre_predecessor = predecessor - 1;
	return pre_predecessor;
}
