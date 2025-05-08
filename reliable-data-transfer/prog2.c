#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prog2.h"

/* ******************************************************************
   ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
   are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
   or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
   (although some can be lost).
 **********************************************************************/

sender_state *a_state;
receiver_state *b_state;

// Helpers
int easy_checksum(pkt_t packet)
{
	int retval = packet.seqnum + packet.acknum;
	for (int i = 0; i < MAX_SIZE; ++i) retval += packet.payload[i];
	return retval;
}

void save_last_sent_packet(pkt_t packet)
{
	a_state->tmp_packet.seqnum = packet.seqnum;
	a_state->tmp_packet.acknum = packet.acknum;
	a_state->tmp_packet.checksum = packet.checksum;
	for (int i = 0; i < 20; ++i)
		a_state->tmp_packet.payload[i] = packet.payload[i];
}


// As per the assignment specification, 
// we can just decline to give a message if we are still waiting 
// for the acknowledgement of a previous one.

/* called from layer 5, passed the data to be sent to other side */
void A_output(msg_t message)
{
	if (a_state->awaiting_reply) {
		return;
	}

	// Make the packet
	pkt_t packet = {++a_state->seq_num, 0, 0, 0};

	for (int i = 0; i < 20; ++i)
		packet.payload[i] = message.data[i];

	packet.checksum = easy_checksum(packet);

	if (TRACE >= 2) {
		printf(" %s | sending packet seqnum:%d acknum:%d checksum:%d payload:", __func__, packet.seqnum, packet.acknum, packet.checksum);
		for (int i = 0; i < 20; ++i) printf("%c", packet.payload[i]);
		printf("\n");
	}
	
	// Send the packet
	save_last_sent_packet(packet);	
	a_state->awaiting_reply = 1;

	tolayer3(0, packet);
}

void B_output(msg_t message)  /* need be completed only for extra credit */
{

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(pkt_t packet)
{
	// What packet did they acknowledge? If they acknowledged the packet we last sent, then we are good.
	if (packet.seqnum == a_state->seq_num) {
		a_state->awaiting_reply = 0;
		return;
	} else {
		// Resend the packet	
		tolayer3(0, a_state->tmp_packet);	
		a_state->awaiting_reply = 1;
	}

	// Do I need to check the checksum? I am really just checking if the packet's seqnum corresponds
	// to the seqnum of our last sent packet.
}

/* called when A's timer goes off */
void A_timerinterrupt()
{

}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	a_state = (sender_state *) malloc(sizeof(sender_state));
	a_state->seq_num = -1;

	a_state->tmp_packet = (pkt_t) {0,0,0,0};
	a_state->awaiting_reply = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(pkt_t packet)
{	
	pkt_t ack_packet = {0, b_state->last_acknowledged_packet, 0, 0};
	
	int corrupt = 0;
	// corruption if they send the wrong number
	if (packet.seqnum != b_state->last_acknowledged_packet + 1) 
		corrupt = 1;

	// corruption if checksum doesnt work
	if (easy_checksum(packet) != packet.checksum) 
		corrupt = 1;

	if (!corrupt) {
		b_state->last_acknowledged_packet = packet.seqnum;

		// send the message upstairs

		tolayer5(1, packet.payload);
	}

	// send the acknowledgement
	tolayer3(1, ack_packet);
}

/* called when B's timer goes off */
void B_timerinterrupt()
{
	
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	b_state = (receiver_state *) malloc(sizeof(receiver_state));
	b_state->last_acknowledged_packet = -1;
}

// Main method do simulate network events. Shouldn't have to look at this.
int main()
{
	struct event *eventptr;
	struct msg  msg2give;
	struct pkt  pkt2give;

	int i,j;
	char c; 

	init();
	A_init();
	B_init();

	while (1) {
		eventptr = evlist;            /* get next event to simulate */
		if (eventptr==NULL)
			goto terminate;
		evlist = evlist->next;        /* remove this event from event list */
		if (evlist!=NULL)
			evlist->prev=NULL;
		if (TRACE>=2) {
			printf("\nEVENT time: %f,",eventptr->evtime);
			printf("  type: %d",eventptr->evtype);
			if (eventptr->evtype==0)
				printf(", timerinterrupt  ");
			else if (eventptr->evtype==1)
				printf(", fromlayer5 ");
			else
				printf(", fromlayer3 ");
			printf(" entity: %d\n",eventptr->eventity);
		}
		time = eventptr->evtime;        /* update time to next event time */
		if (nsim==nsimmax)
			break;                        /* all done with simulation */
		if (eventptr->evtype == FROM_LAYER5 ) {
			generate_next_arrival();   /* set up future arrival */
			/* fill in msg to give with string of same letter */    
			j = nsim % 26; 
			for (i=0; i<20; i++)  
				msg2give.data[i] = 97 + j;
			if (TRACE>2) {
				printf("          MAINLOOP: data given to student: ");
				for (i=0; i<20; i++) 
					printf("%c", msg2give.data[i]);
				printf("\n");
			}
			nsim++;
			if (eventptr->eventity == A) 
				A_output(msg2give);  
			else
				B_output(msg2give);  
		}
		else if (eventptr->evtype ==  FROM_LAYER3) {
			pkt2give.seqnum = eventptr->pktptr->seqnum;
			pkt2give.acknum = eventptr->pktptr->acknum;
			pkt2give.checksum = eventptr->pktptr->checksum;
			for (i=0; i<20; i++)  
				pkt2give.payload[i] = eventptr->pktptr->payload[i];
			if (eventptr->eventity ==A)      /* deliver packet by calling */
				A_input(pkt2give);            /* appropriate entity */
			else
				B_input(pkt2give);
			free(eventptr->pktptr);          /* free the memory for packet */
		}
		else if (eventptr->evtype ==  TIMER_INTERRUPT) {
			if (eventptr->eventity == A) 
				A_timerinterrupt();
			else
				B_timerinterrupt();
		}
		else  {
			printf("INTERNAL PANIC: unknown event type \n");
		}
		free(eventptr);
	}

terminate:
	printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
}

