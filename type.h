//------------------------------------------------------------------------------
typedef short typId;

typedef struct bcast_t {
	int len;
	char type; //'m'
	typId id_process;
	int seq;
	int ack;
	double startMiliSeconds;
	char payload[2]; //payload bytes to increase the size of packet.
} __attribute__((packed)) bcast;
//------------------------------------------------------------------------------
typedef struct ack_t {
	int len;
	char type;//'a'
	typId id_process;
	int seq;
	int revNum;
}  __attribute__((packed)) ack;
//------------------------------------------------------------------------------
typedef struct ok_t {
	int len;
	char type;//'y','n'
}  __attribute__((packed)) ok;
//------------------------------------------------------------------------------
typedef union {
	bcast b;
	ack   a;
	ok    o;
} msg;
//------------------------------------------------------------------------------
