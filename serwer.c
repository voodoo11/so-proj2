/*******************************************
*	Jakub Kowalski
*	nr indeksu: 334674
*	mail: jk334674@students.mimuw.edu.pl
********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/msg.h>
#include <stdbool.h>
#include "err.h"
#include "mesg.h"
#include <errno.h>

#define MAX_RES 	9999
#define MAX_TYPES	99

typedef struct {
	struct cMesg first;
	struct cMesg second;
} pair;

/* monitor */
pthread_cond_t for_second[MAX_TYPES];
pthread_cond_t for_resources[MAX_TYPES];
pthread_cond_t for_first[MAX_TYPES];
pthread_mutex_t mutex;
pthread_mutexattr_t attr;
bool waiting_for_second[MAX_TYPES];
bool waiting_for_resources[MAX_TYPES];
bool awake[MAX_TYPES];
int resources[MAX_TYPES];
int need;
int in_qid, out_qid, end_qid;
pair ready[MAX_TYPES];
/* monitor */

void exit_server(int sig) {
	rmMesgQueues(in_qid, out_qid, end_qid);
	exit(0);
}

void initialize(int res_types, int res_num) {
	int error;
	int i;

	/* stworz kolejki do komunikacji */
	makeQueues(&in_qid, &out_qid, &end_qid);

	/* inicjalizuj mutexy */
	if((error = pthread_mutexattr_init(&attr)) != 0) {
		rmMesgQueues(in_qid, out_qid, end_qid);
		syserr(error, "Mutex attr init.\n");
	}
	if((error = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL)) != 0) {
		rmMesgQueues(in_qid, out_qid, end_qid);
		syserr(error, "Mutex attr settype.\n");
	}	
	if((error = pthread_mutex_init(&mutex, &attr)) != 0) {
		rmMesgQueues(in_qid, out_qid, end_qid);
		syserr(error, "Mutex init.\n");
	}

	/* inicjalizacja zmiennych warunokwych */
	for(i = 0; i < res_types; ++i) {
		if((error = pthread_cond_init(&for_second[i], 0)) != 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "Cond init (for_second).\n");
		}
		if((error = pthread_cond_init(&for_resources[i], 0)) != 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "Cond init (for_second).\n");
		}
		if((error = pthread_cond_init(&for_first[i], 0)) != 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "Cond init (for_second).\n");
		}

		waiting_for_second[i] = false;
		waiting_for_resources[i] = false;
		awake[i] = false;
		resources[i] = res_num;
	}
}

void *thread(void* order) {
	struct cMesg client;
	struct cMesg mate;

	struct cMesg* temp = (struct cMesg*) order;
	client = *temp;
	free(temp);

	sMesg start_mesg;
	cReply end_mesg;
	bool first = false;
	int type = client.res_type-1;
	int error;
	int msg_size;

	/* wejdz do monitora */
	if((error = pthread_mutex_lock(&mutex)) != 0) {
		rmMesgQueues(in_qid, out_qid, end_qid);
		syserr(error, "Mutex lock.\n");
	}

	/* ktos juz czeka na miejsce dla nowej pary */
	while(waiting_for_resources[type]) {
		if((error = pthread_cond_wait(&for_first[type], &mutex)) != 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "Wait for_first.\n");
		}
	}

	if(waiting_for_second[type]) {
		ready[type].second = client;
		mate = ready[type].first;

		/* musze sprawdzic, czy sa dostepne zasoby */
		while((mate.res_num + client.res_num) > resources[type]) {
			waiting_for_resources[type] = true;
			need = mate.res_num + client.res_num;
			if((error = pthread_cond_wait(&for_resources[type], &mutex)) != 0) {
				rmMesgQueues(in_qid, out_qid, end_qid);
				syserr(error, "Wait for_resources.\n");
			}
		}

		/* doczekalem sie na zasoby, wyjmuje z puli */
		waiting_for_resources[type] = false;
		resources[type] = resources[type] - mate.res_num - client.res_num;

		printf("Wątek %u przydziela %d+%d zasobów %d klientom %ld %ld"
			", pozostało %d zasobów\n",
			(unsigned int)pthread_self(), mate.res_num, client.res_num,
			type+1, mate.mesg_type, client.mesg_type, resources[type]
		);

		/* budze drugiego z pary */
		waiting_for_second[type] = false;
		/* pozwala na opuszczenie petli przez partnera we wlasciwym momencie */
		awake[type] = true;
		if((error = pthread_cond_signal(&for_second[type])) != 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "Signal for_second.\n");
		}

	} else {
		first = true;
		/* pierwszy, budze kolejke i czekam na pare */
		if((error = pthread_cond_signal(&for_first[type])) != 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "Waiting for_first.\n");
		}
		while(!waiting_for_second[type] && !awake[type]) {
			ready[type].first = client;
			waiting_for_second[type] = true;
			if((error = pthread_cond_wait(&for_second[type], &mutex)) != 0) {
				rmMesgQueues(in_qid, out_qid, end_qid);
				syserr(error, "Waiting for_second.\n");
			}
		}

		awake[type] = false;

		mate = ready[type].second;

		if((error = pthread_cond_signal(&for_first[type])) != 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "Signal for_first.\n");
		}
	}

	if((error = pthread_mutex_unlock(&mutex)) != 0) {
		rmMesgQueues(in_qid, out_qid, end_qid);
		syserr(error, "Unlock mutex.\n");
	}

	start_mesg.mesg_type = client.mesg_type;
	start_mesg.mate_pid = mate.mesg_type;

	/* klient musi znac typ wiadomosci, ktory odesle. Jest nim konkatenacja
	pidow pierwszego i drugiego klienta z pary*/
	if(first) {
		start_mesg.reply_key = client.mesg_type*100000 + mate.mesg_type;
	} else {
		start_mesg.reply_key = mate.mesg_type*100000 + client.mesg_type;
	}

	msg_size = sizeof(sMesg) - sizeof(long);
	if((error = msgsnd(out_qid, &start_mesg, msg_size, 0)) != 0) {
		rmMesgQueues(in_qid, out_qid, end_qid);
		syserr(error, "Msg (start_mesg) send.\n");
	}

	/* tylko watek pierwszego klienta czeka na skonczenie pracy
	przez obydwu klientow */
	if(first) {
		/* czekam na zakonczenie klienta */
		msg_size = sizeof(cReply) - sizeof(long);
		if((error = msgrcv(end_qid, &end_mesg, msg_size,
				start_mesg.reply_key, 0)) <= 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "Msg (end_mesg) rcv.\n");
		}

		/* czekam na zakonczenie partnera */
		if((error = msgrcv(end_qid, &end_mesg, msg_size,
				start_mesg.reply_key, 0)) <= 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "Msg (end_mesg) rcv.\n");
		}

		/* para skonczyla, zwalniam zajete zasoby */
		/* wejdz do monitora */
		if((error = pthread_mutex_lock(&mutex)) != 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "Mutex lock.\n");
		}

		resources[type] = resources[type] + client.res_num + mate.res_num;
		if(need <= resources[type]) {
			if((error = pthread_cond_signal(&for_resources[type])) != 0) {
				rmMesgQueues(in_qid, out_qid, end_qid);
				syserr(error, "Signal for_resources.\n");
			}
		}

		/* zwalniam monitor */
		if((error = pthread_mutex_unlock(&mutex)) != 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "Unlock mutex.\n");
		}
	}

	return (void*) 0;
}

int main(int argc, char *argv[]) {
	int res_types = 0; 				/* ilosc typow zasobow */
	int res_num = 0;				/* ilosc egzemplarzy kazdego z zasobow */
	int error;
	int msg_size = sizeof(struct cMesg) - sizeof(long);
	pthread_t thr;
	pthread_attr_t attr;

	/* obsługa SIGINT */
	if (signal(SIGINT,  exit_server) == SIG_ERR) {
    	syserr(0, "signal.\n");
    }

    /* poprawna ilosc argumentow */
	if((error = argc) < 2) {
		syserr(error, "Too few arguments.\n");
	}

	res_types = atoi(argv[1]);
	res_num = atoi(argv[2]);

	/* inicjalizacja tablic, kolejek, mutexow i zmiennych warunkowych */
	initialize(res_types, res_num);

	for(;;) {
		/* czekanie na zamowienie od klienta */
		struct cMesg *order = malloc(sizeof(struct cMesg));
		
		if((error = msgrcv(in_qid, order, msg_size, 0l, 0)) <= 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "Mesg rcv");
		}
		if((error = pthread_attr_init(&attr)) != 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "thread attr init.\n");
		}
		if((error = pthread_attr_setdetachstate(&attr, 
			PTHREAD_CREATE_DETACHED)) != 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "Set detach.\n");
		}
		/* stworz watek, ktory bedzie obslugiwal danego klienta */
		if((error = pthread_create(&thr, &attr, thread, order)) != 0) {
			rmMesgQueues(in_qid, out_qid, end_qid);
			syserr(error, "Thread create.\n");
		}
	}
	
	rmMesgQueues(in_qid, out_qid, end_qid);

	return 0;
}