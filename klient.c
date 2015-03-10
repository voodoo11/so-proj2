/*******************************************
*	Jakub Kowalski
*	nr indeksu: 334674
*	mail: jk334674@students.mimuw.edu.pl
********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/msg.h>
#include "err.h"
#include "mesg.h"

int main(int argc, char *argv[]) {
	int in_qid;
	int out_qid;
	int end_qid;
	int error;
	int wait_time;
	int msg_size;
	sMesg start_mesg;
	cReply end_mesg;
	struct cMesg msg;

	if((error = argc) < 3) {
		syserr(error, "Too few arguments.\n");
	}

	msg.mesg_type = getpid();
	msg.res_type = atoi(argv[1]);
	msg.res_num = atoi(argv[2]);
	wait_time = atoi(argv[3]);

	/*otwarcie kolejek*/
	getQueues(&in_qid, &out_qid, &end_qid);

	/*wyslanie zamowienia do serwera*/
	msg_size = sizeof(struct cMesg) - sizeof(long);
	if((error = msgsnd(out_qid, &msg, msg_size, 0)) != 0) {
		syserr(error, "Msg send.\n");
	}

	/*czekanie na rozpoczecie pracy*/
	msg_size = sizeof(sMesg) - sizeof(long);
	if((error = msgrcv(in_qid, &start_mesg, msg_size, getpid(), 0)) <= 0) {
		syserr(error, "Msg (start_mesg) rcv.\n");
	}

	end_mesg.mesg_type = start_mesg.reply_key;
	end_mesg.mesg = '!';

	/*wypisanie danych*/
	printf("%d %d %d %d\n", 
			msg.res_type, msg.res_num, getpid(), start_mesg.mate_pid);
	fflush(stdout);

	sleep(wait_time);

	/*wyslanie serwerowi wiadomosci o koncu pracy*/
	msg_size = sizeof(cReply) - sizeof(long);
	if((error = msgsnd(end_qid, &end_mesg, msg_size, 0)) != 0) {
		syserr(error, "Msg (end_mesg) send.\n");
	}

	printf("%d KONIEC", getpid());

	return 0;
}