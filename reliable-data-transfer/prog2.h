#ifndef RDT_H
#define RDT_H

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

#define MAX_SIZE 20

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
typedef struct msg {
	char data[MAX_SIZE];
} msg_t;

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
typedef struct pkt {
	int seqnum;
	int acknum;
	int checksum;
	char payload[MAX_SIZE];
} pkt_t;

// For the sender side.
typedef struct rdt_sender_state {
	int seq_num;
	pkt_t tmp_packet; 
	int awaiting_reply;
} sender_state;

typedef struct rdt_receiver_state {
	int last_acknowledged_packet;
} receiver_state;

// Helper functions provided by Kurose

void stoptimer(int AorB);

void starttimer(int AorB, float increment);

void tolayer3(int AorB, pkt_t packet);

// datasend has size 20 expected
void tolayer5(int AorB, char *datasent);

// Functions required for network event simulation

struct event;
void init(); /* initialize the simulator */
void printevlist();
void insertevent(struct event *p);
void generate_next_arrival();

#endif //RDT_H
