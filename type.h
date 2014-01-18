#define MSG_HEADER_SIZE (sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int) + sizeof(double))
//------------------------------------------------------------------------------
typedef struct msg {
	char type; //'m'
	int id_process;
	int len;
	int seq;
	int dest;
	int ack;
	double startMiliSeconds;
	char payload[2]; //payload bytes to increase the size of packet.
} __attribute__((packed)) msg;
//------------------------------------------------------------------------------
typedef struct ack {
	char type;//'m'
	int id_process;
	int seq;
	int revNum;
	int confNum;
}  __attribute__((packed)) ack;
//------------------------------------------------------------------------------
typedef struct ok {
	char ok;//'y','n'
}  __attribute__((packed)) ack;
//------------------------------------------------------------------------------
