/*******************************************
*	Jakub Kowalski
*	nr indeksu: 334674
*	mail: jk334674@students.mimuw.edu.pl
********************************************/

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "mesg.h"
#include "err.h"

/*tworzy dwie kolejki
pierwsza do odbioru wiadomości od klientów
druga do wysyłania wiadomosci do klientow
*/
void makeQueues(int *in_qid, int *out_qid, int *end_qid) {
	if((*out_qid = msgget(OUT_KEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1) {
		syserr(0, "Making first (out) queue.\n");
	}
	if((*in_qid = msgget(IN_KEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1) {
		syserr(0 ,"Making second (in) queue.\n");
	}
	if((*end_qid = msgget(END_KEY, 0666 | IPC_CREAT | IPC_EXCL)) == -1) {
		syserr(0 ,"Making third (end) queue.\n");
	}
}

void getQueues(int *in_qid, int *out_qid, int *end_qid) {
	if((*in_qid = msgget(OUT_KEY, 0)) == -1) {
		syserr(0, "Making first (out) queue.\n");
	}
	if((*out_qid = msgget(IN_KEY, 0)) == -1) {
		syserr(0 ,"Making second (in) queue.\n");
	}
	if((*end_qid = msgget(END_KEY, 0)) == -1) {
		syserr(0 ,"Making third (end) queue.\n");
	}
}

void rmMesgQueues(int in_qid, int out_qid, int end_qid) {
	int error;
	if((error = msgctl(in_qid, IPC_RMID, 0)) != 0) {
		syserr(error, "Remove out mesg queue.\n");
	}
	if((error = msgctl(out_qid, IPC_RMID, 0)) != 0) {
		syserr(error, "Remove in mesg queue.\n");
	}
	if((error = msgctl(end_qid, IPC_RMID, 0)) != 0) {
		syserr(error, "Remove in mesg queue.\n");
	}
}