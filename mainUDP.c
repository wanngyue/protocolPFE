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
} config_info;

void validate_input_parameters(int argc, char **argv);
void parse_config_file(int self_id, const char * config_file_name, int num_nodes);
void get_ip_port(const char* config_file, config_info * config);
void initialize();

config_info *config_sample;
char * config_file = "config_file.dat";

int self_id, num_nodes, time_out;
int interval_preparation, interval_warmingUp, interval_mesure, interval_stop;

pthread_t th1, th2, th3;
void *ret;
static sem_t my_sem;

int seq_num, ack_num, rev_num, conf_num;
msg *msg_sample;
char IP_broadcast[16];

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

	sem_init (&my_sem, 0, 0);

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
		//broadcast -> OK
		sleep(interval_warmingUp);
		//start timer
		sleep(interval_mesure);
		//stop timer
		sleep(interval_stop);
		//broadcast -> stop
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

			config->nodes[node_counter].socket_broadcast = -1;
			config->nodes[node_counter].socket_recv = -1

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
void initialize(){

	seq_num = 0;
	ack_num = 0;
	rev_num = 0;
	conf_num = 0;

	config_sample->nodes[config_sample->self_id].socket_broadcast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	int on = 1;
	if(setsockopt(config_sample->nodes[config_sample->self_id].socket_broadcast,
			SOL_SOCKET, SO_BROADCAST, (int *)&on, sizeof(on)) < 0) {

		perror("setsockopt");
	}
}
void *process_msg (void * arg){
//  int i;
//
//  for (i = 0 ; i < 5 ; i++) {
//    printf ("Thread %s: %d\n", (char*)arg, i);
//    sleep (1);
//  }
	int status;
	char msg_type;

	while(1){
		//if(config_sample->self_id != 0){
		status = recv(config_sample->nodes[0].socket_id, &msg_type, sizeof(char), MSG_PEEK);
		//printf("msg_type = %c\n",msg_type);
		assert(status == sizeof(char));
		switch(msg_type){
			case'y':
				printf("Preparation is done\n");
				sem_post(&my_sem);
				break;
			case'm':
				handle_data();
				break;
			case'a':
				handle_ack();
				break;
			case's':
				exit(EXIT_SUCCESS);
				printf("Message type 'stop' received.%c\n", msg_type);
				break;
			default: {
				printf("Unexpected message type received.%c\n", msg_type);
			}
		}
		//}
	}

	pthread_exit (0);
}
void *send_msg (void * arg){
//  int i;
//
//  for (i = 0 ; i < 5 ; i++) {
//    printf ("Thread %s: %d\n", (char*)arg, i);
//    sleep (1);
//  }
	if(config_sample->self_id == 0){
		char yes = 'y';
		printf("Send OK to proposers\n");
		send(config_sample->nodes[1].socket_id, &yes, sizeof(char), 0);
		//send(config_sample->nodes[2].socket_id, &yes, sizeof(char), 0);
	}else{
		sem_wait (&my_sem);
		while(1){
			seq_num++;
			broadcastData(config_sample,seq_num, ack_num, msg_sample);
			sleep(time_out);
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
	/*
	 * TO DO
	 */
	pthread_exit (0);
}
void broadcast(msg *message){
	struct sockaddr_in s;

	if(config_sample->nodes[config_sample->self_id].socket_broadcast < 0)
		return;

	memset(&s, '\0', sizeof(struct sockaddr_in));
	s.sin_family = AF_INET;
	s.sin_port = htons(config_sample->nodes[config_sample->self_id].port);
	s.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	printf("broadcast a message\n");
	if(sendto(config_sample->nodes[config_sample->self_id].socket_broadcast, message, strlen(message), 0, (struct sockaddr *)&s, sizeof(struct sockaddr_in)) < 0)
		perror("sendto");
}
