//------------------------------------------------------------------------------
typedef short typId;

typedef struct bcast_t {
	char type; //'m'
	typId id_process;
	int seq;
	int ack;
	double startMiliSeconds;
	char payload[2]; //payload bytes to increase the size of packet.
} __attribute__((packed)) mcast;
//------------------------------------------------------------------------------
typedef struct ack_t {
	char type;//'a'
	typId id_process;
	int seq;
	int revNum;
}  __attribute__((packed)) ack;
//------------------------------------------------------------------------------
typedef struct ok_t {
	char type;//'y','n'
	char id;
}  __attribute__((packed)) ok;
//------------------------------------------------------------------------------
typedef union {
	mcast m;
	ack   a;
	ok    o;
} msg;
//------------------------------------------------------------------------------
