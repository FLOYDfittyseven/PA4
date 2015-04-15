#include <pthread.h>

typedef enum { false, true } bool;

struct account
{
	char accountname[101];
	float balance;
	bool flag;
};

struct account *
CreateAccount( char * accountname );

void
DestroyAccount( struct account * a );

int
create_bank_account( struct account ** bank, pthread_mutex_t ** mutexes, char * accountname, int in_session, char ** message );

int
serve_account( struct account ** bank, pthread_mutex_t ** mutexes, char * accountname, int * in_session, char ** message, char ** session_name );

int
deposit( struct account ** bank, char * accountname, float amount );

int
withdraw( struct account ** bank, char * accountname, float amount );

float
query( struct account ** bank, char * accountname );

int
end( struct account ** bank, char * accountname );
