/******************************************************************************/
/*                                                                            */
/* ENTITY IMPLEMENTATIONS                                                     */
/*                                                                            */
/******************************************************************************/



#include <stdio.h>
#include "simulator.h"
#include <stdbool.h>

int windowSize = 100;
float rtt = 1000;

struct sProp{
	int base;
	int nextSeqNum;
	struct pkt packets[100];
};

struct rProp{
	int expectedSeqNum;
};

struct sProp sender;
struct rProp receiver;

int checksum(struct pkt packet);

bool notCorrupt(struct pkt packet);



/**** A ENTITY ****/

// This routine will be called once by the simulator before any other entity
// "A" routines are called. This can be used to initialize any state if needed.
void A_init() {
	sender.base = 0;
	sender.nextSeqNum = 0;
}

// This function is called when layer 5 on the "A" entity has data that should
// be sent to entity "B". That is, the passed in `message` is being output from
// entity "A" and should be passed over the network to entity "B".
void A_output(struct msg message) {

	if(sender.nextSeqNum < sender.base + windowSize){
		// Create a packet to encapsulate message
		struct pkt packet;

		//make_pkt(nextseqnum, data, chksum)
		packet.length = message.length;
		packet.seqnum = sender.nextSeqNum;

		// Copy data from the message to the packet payload
		for(int i = 0; i < 20; i++){
			packet.payload[i] = message.data[i];
		}

		packet.checksum = checksum(packet);

		sender.packets[sender.nextSeqNum] = packet;

		// udt_sent[sndpkt[nextseqnum])
		tolayer3_A(packet);

		if(sender.base == sender.nextSeqNum){
			starttimer_A(rtt);
		}

		sender.nextSeqNum++;
	}
}

// This function is called when a packet arrives from the network destined for
// entity "A". That is, the `packet` is passed from layer 3 to layer 4 and is
// being input to entity "A".
void A_input(struct pkt packet) {

	//rdt_rcv(rcvpkt) && notcorrupt(rcvpkt)
	if(notCorrupt(packet)){

		sender.base = packet.acknum + 1;

		if(sender.base == sender.nextSeqNum){
			stoptimer_A();
		}else{
			starttimer_A(rtt);
		}
	}
}

// This function will be called when entity "A"'s timer has fired.
void A_timerinterrupt() {
	starttimer_A(rtt);

	//Resent all packets from sndpkt[base] to sndpkt[nextseqnum-1]
	for(int i = sender.base; i < sender.nextSeqNum; i++){
		tolayer3_A(sender.packets[i]);
	}

}


/**** B ENTITY ****/

// This routine will be called once by the simulator before any other entity
// "B" routines are called. This can be used to initialize any state if needed.
void B_init() {
	receiver.expectedSeqNum = 0;
}

// This function is called when a packet arrives from the network destined for
// entity "B". That is, the `packet` is passed from layer 3 to layer 4 and is
// being input to entity "B".
void B_input(struct pkt packet) {
	//rdt_rcv(rcvpkt) && notcorrupt(rcvpkt) && hasseqnum(rcvpkt, expectedseqnum)
	if(notCorrupt(packet) && packet.seqnum == receiver.expectedSeqNum){
		//extract(rcvpkt, data)
		struct msg message;
		message.length = packet.length;
		for(int i = 0; i < 20; i++){
			message.data[i] = packet.payload[i];
		}

		//deliver_data(data)
		tolayer5_B(message);

		//sndpkt = make_pkt(expectedseqnum, ACK, chksum)
		struct pkt ACK;
		ACK.acknum = receiver.expectedSeqNum;
		ACK.checksum = packet.checksum;

		//udt_send(sndpkt)
		tolayer3_B(ACK);

		receiver.expectedSeqNum++;
	}
}

// This function will be called when entity "B"'s timer has fired.
void B_timerinterrupt() {
}



int checksum(struct pkt packet){
	int sum = 0;

	sum += packet.seqnum;
	sum += packet.acknum;

	for(int i = 0; i < 20; i++){
		sum+= packet.payload[i];
	}
	return sum;
}

bool notCorrupt(struct pkt packet){
	return (checksum(packet) == packet.checksum);
}
