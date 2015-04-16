#include <pthread.h>

typedef enum { false, true } bool;

struct account
{
	char *accountname;
	float balance;
	bool flag;
	pthread_mutex_t mutex;
};

struct bank
{
	struct account **accounts;
	struct account **emptyslot;
	struct account *active;
	int numAccounts;
	pthread_mutex_t mutex;
};

struct account *
CreateAccount( char * accountname );

void
DestroyAccount( struct account * a );

struct bank *
CreateBank();

void
DestroyBank( struct bank * b );

struct account *
FindAccount( char * name, struct bank * Bank );

int
create_bank_account( struct bank * Bank, char * accountname, int in_session, char ** message );

int
serve_account( struct bank * Bank, char * accountname, int * in_session, char ** message, char ** session_name );

int
deposit( struct bank * Bank, float amount, char ** message );

int
withdraw( struct bank * Bank, float amount, char ** message );

float
query( struct account ** bank, char * accountname );

int
end( struct bank * Bank, int * in_session, char ** message, char * session_name );
