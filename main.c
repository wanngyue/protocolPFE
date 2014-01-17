#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<assert.h>

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
//int get_substr(int start_index, int end_index, char * source, char * result_to_return);

config_info *config_sample;
char * config_file = "config_file.dat";

int self_id, num_nodes, time_out;
int interval_preparation, interval_warmingUp, interval_mesure, interval_stop;

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


	/*
	adjust_position(get_config_info());

	if (am_i_leader == 1)
		create_tcp_server_socket_for_star(get_config_info()->l.IP, get_config_info()->l.port, get_config_info()->num_nodes, get_config_info()->l.sid);
	else {
		create_ring(get_config_info());
		connect_to_leader(get_config_info());
		//		setsock_nonblock(config_sample->nodes[self_id].self_sid);
		//		setsock_nonblock(config_sample->nodes[self_id].successor_sid);
	}
	*/
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
