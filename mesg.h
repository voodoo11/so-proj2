/*******************************************
*	Jakub Kowalski
*	nr indeksu: 334674
*	mail: jk334674@students.mimuw.edu.pl
********************************************/

struct cMesg {
	long mesg_type;
	int res_type;
	int res_num;
};

typedef struct {
	long mesg_type;
	pid_t mate_pid;
	long reply_key;
} sMesg;

typedef struct {
	long mesg_type;
	char mesg;
} cReply;

void makeQueues(int *in_qid, int *out_qid, int *end_qid);
void getQueues(int *in_qid, int *out_qid, int *end_qid);
void rmMesgQueues(int in_qid, int out_qid, int end_qid);

#define IN_KEY 	1234L
#define OUT_KEY 1235L
#define END_KEY 1236L