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
	int numAccounts;
	pthread_mutex_t mutex;
};

/*
 * CreateAccount takes the name of the account as an argument. It
 * allocates the necessary memory for a struct account and initializes 
 * the fields. If successful, it returns a pointer to the allocated 
 * struct. Otherwise, it returns NULL.
 */
struct account *
CreateAccount( char * accountname );

/* DestroyAccount takes a pointer to a struct account created with
 * CreateAccount. It frees the appropriate memory and returns nothing.
 */
void
DestroyAccount( struct account * a );

/* CreateBank allocates memory for a struct bank and initializes its
 * fields. If successful, it returns a pointer to the memory. Otherwise,
 * it returns NULL.
 */
struct bank *
CreateBank();

/* DestroyBank takes a pointer to a struct bank created with CreateBank
 * and frees the appropriate memory chunks. It returns nothing.
 */
void
DestroyBank( struct bank * b );

/* FindAccount takes the name of the desired account and a pointer to
 * bank. It checks each account in the bank's list of accounts for name.
 * If the function finds an account with the given name, it returns a 
 * pointer to the account. Otherwise, it returns NULL.
 */ 
struct account *
FindAccount( char * name, struct bank * Bank );

/*
 * create_bank_account takes a pointer to the bank, the name of the
 * account to be created, the in_session flag, and a pointer to the
 * message buffer. It locks the bank's mutex while creating the account
 * and adding it to the bank's list of accounts, then unlocks the mutex.
 * If successful, it writes the appropriate message to the message buffer
 * and returns the number of characters written. Otherwise, it returns -1.
 */
int
create_bank_account( struct bank * Bank, char * accountname, int in_session, char ** message );

/* serve_account takes a pointer to the bank, the name of the account to
 * be served, a pointer to the in_session flag, a pointer to the message
 * buffer, a pointer to the session_name buffer, a pointer to waiting_flag,
 * and a double pointer to the active account.
 * The function locks the bank's mutex while looking for the given account,
 * then unlocks the mutex. If the account is found in the bank, the function
 * locks the individual account's mutex before making the necessary
 * arrangements for a customer session. If successful, it writes the 
 * appropriate message to the message buffer and returns the number of 
 * characters written. Otherwise, it returns -1.
 */
int
serve_account( struct bank * Bank, char * accountname, int * in_session, char ** message, char ** session_name, bool * waiting_flag, struct account ** active );

/* deposit takes a pointer to the bank, the amount to be deposited, a 
 * pointer to the message buffer, and a double pointer to the active
 * account. It simply adds amount to the active account's balance. If
 * successful, the function writes the 
 * appropriate message to the message buffer and returns the number of 
 * characters written. Otherwise, it returns -1.
 */
int
deposit( struct bank * Bank, float amount, char ** message, struct account ** active );

/* withdraw takes a pointer to the bank, the amount to be deposited, a 
 * pointer to the message buffer, and a double pointer to the active
 * account. It simply subtracts amount from the active account's balance.
 * If successful, the function writes the appropriate message to the
 * message buffer and returns the number of 
 * characters written. Otherwise, it returns -1.
 */
int
withdraw( struct bank * Bank, float amount, char ** message, struct account ** active );

/* query takes a pointer to the message buffer, the in_session flag, and
 * a pointer to the active account. It simply writes the appropriate
 * message to the message buffer and returns the number of characters
 * written.
 */
int
query( char ** message, int in_session, struct account * active );

/* end takes a pointer to the bank, a pointer to the in_session flag, a 
 * pointer to the message buffer, the name of the session to be ended,
 * and a double pointer to the active account. The function makes the
 * necessary arrangements for ending an active session, then unlocks
 * the account's mutex. If successful, it writes the 
 * appropriate message to the message buffer and returns the number of 
 * characters written. Otherwise, it returns -1.
 */
int
end( struct bank * Bank, int * in_session, char ** message, char * session_name, struct account ** active );

/* quit takes the in_session flag, a pointer to the active account, and
 * the socket descriptor. If the client is currently in a customer 
 * session, the session is closed and the account's mutex is unlocked.
 * The function closes the socket and calls pthread_exit.
 */
void
quit( int in_session, struct account * active, int sd );
