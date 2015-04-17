#include "bank.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

/*
 * CreateAccount takes the name of the account as an argument. It
 * allocates the necessary memory for a struct account and initializes 
 * the fields. If successful, it returns a pointer to the allocated 
 * struct. Otherwise, it returns NULL.
 */
struct account *
CreateAccount( char * accountname )
{
	struct account		*ret;
	
	if( !accountname || sizeof(accountname) > 101 )
	{
		return NULL;
	}
	
	if( (ret = (struct account *)malloc( sizeof(struct account) )) == 0)
	{
		printf( "malloc() failed in %s line %d.\n", __FILE__, __LINE__);
		return NULL;
	}
	else if( (ret->accountname = (char *)malloc(101 * sizeof(char) )) == 0)
	{
		printf( "malloc() failed in %s line %d.\n", __FILE__, __LINE__);
		free( ret );
		return NULL;
	}
	
	strcpy(ret->accountname, accountname);
	ret->accountname[100] = '\0';
	ret->balance = 0;
	ret->flag = false;
	pthread_mutex_init( &ret->mutex, NULL );
	
	return ret;
}

/* DestroyAccount takes a pointer to a struct account created with
 * CreateAccount. It frees the appropriate memory and returns nothing.
 */
void
DestroyAccount( struct account * a )
{
	if( !a )
	{
		return;
	}
	free( a->accountname );
	free( a );
}

/* CreateBank allocates memory for a struct bank and initializes its
 * fields. If successful, it returns a pointer to the memory. Otherwise,
 * it returns NULL.
 */
struct bank *
CreateBank()
{
	struct bank * b;
	int i;
	
	if( (b = (struct bank *)malloc( sizeof(struct bank) )) == 0)
	{
		printf( "malloc() failed in %s line %d.\n", __FILE__, __LINE__);
		return NULL;
	}
	else if( (b->accounts = (struct account **)malloc(20 * sizeof(struct account *) )) == 0)
	{
		printf( "malloc() failed in %s line %d.\n", __FILE__, __LINE__);
		return NULL;
	}
	
	for( i=0; i<20; i++ )
	{
		b->accounts[i] = NULL;
	}
	
	b->numAccounts = 0;
	pthread_mutex_init( &b->mutex, NULL );
	b->emptyslot = b->accounts;
	
	return b;
}

/* DestroyBank takes a pointer to a struct bank created with CreateBank
 * and frees the appropriate memory chunks. It returns nothing.
 */
void
DestroyBank( struct bank * b )
{
	if( !b )
	{
		return;
	}
	
	while( b->numAccounts > 0 )
	{
		DestroyAccount( b->accounts[b->numAccounts] );
		b->numAccounts--;
	}
	free( b->accounts );
	free( b );
	return;
}

/* FindAccount takes the name of the desired account and a pointer to
 * bank. It checks each account in the bank's list of accounts for name.
 * If the function finds an account with the given name, it returns a 
 * pointer to the account. Otherwise, it returns NULL.
 */ 
struct account *
FindAccount( char * name, struct bank * Bank )
{
	int i;
	
	for( i = 0; i < Bank->numAccounts; i++ )
	{
		if( strcmp( Bank->accounts[i]->accountname, name ) == 0 )
		{
			return Bank->accounts[i];
		}
	}
	
	return NULL;
}

/*
 * create_bank_account takes a pointer to the bank, the name of the
 * account to be created, the in_session flag, and a pointer to the
 * message buffer. It locks the bank's mutex while creating the account
 * and adding it to the bank's list of accounts, then unlocks the mutex.
 * If successful, it writes the appropriate message to the message buffer
 * and returns the number of characters written. Otherwise, it returns -1.
 */
int
create_bank_account( struct bank * Bank, char * accountname, int in_session, char ** message )
{
	struct account *ptr;
	
	if( !Bank | !accountname )
	{
		return -1;
	}
	else if( in_session )
	{
		return sprintf( *message, "You may not create accounts while in a customer session.\n" );
	}
	
	pthread_mutex_lock( &Bank->mutex );
	
	if( Bank->numAccounts == 20 )
	{
		pthread_mutex_unlock( &Bank->mutex );
		return sprintf( *message, "Bank is full.\n" );
	}
	else if( (ptr = FindAccount( accountname, Bank )) )
	{
		pthread_mutex_unlock( &Bank->mutex );
		return sprintf( *message, "Account \"%s\" already exists.\n", ptr->accountname );
	}
	else
	{
		*(Bank->emptyslot) = CreateAccount( accountname );
		Bank->emptyslot++;
		Bank->numAccounts++;
		pthread_mutex_unlock( &Bank->mutex );
		return sprintf( *message, "Account \"%s\" successfully created.\n", accountname );
	}
}

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
serve_account( struct bank * Bank, char * accountname, int * in_session, char ** message, char ** session_name, bool * waiting_flag, struct account ** active )
{
	struct account *ptr;
	
	if( !Bank | !accountname )
	{
		return -1;
	}
	else if( *in_session )
	{
		return sprintf( *message, "You must end the current customer session (%s) before serving an account.\n", *session_name );
	}
	else if( (pthread_mutex_lock( &Bank->mutex )), !(ptr = FindAccount( accountname, Bank )) )
	{
		pthread_mutex_unlock( &Bank->mutex );
		return sprintf( *message, "Account \"%s\" does not exist.\n", accountname );
	}
	else
	{
		pthread_mutex_unlock( &Bank->mutex );
		
		if( pthread_mutex_trylock( &ptr->mutex )  )
		{
			*waiting_flag = true;
			return sprintf( *message, "Waiting to start customer session for account \"%s\".\n", ptr->accountname );
		}
		*waiting_flag = false;
		*in_session = 1;
		strcpy( *session_name, accountname );
		ptr->flag = true;
		*active = ptr;
		return sprintf( *message, "Now serving account \"%s\".\n", accountname );
	}
}

/* end takes a pointer to the bank, a pointer to the in_session flag, a 
 * pointer to the message buffer, the name of the session to be ended,
 * and a double pointer to the active account. The function makes the
 * necessary arrangements for ending an active session, then unlocks
 * the account's mutex. If successful, it writes the 
 * appropriate message to the message buffer and returns the number of 
 * characters written. Otherwise, it returns -1.
 */
int
end( struct bank * Bank, int * in_session, char ** message, char * session_name, struct account ** active )
{
	if( !Bank )
	{
		return -1;
	}
	else if( !(*in_session) )
	{
		return sprintf( *message, "You do not currently have an active customer session.\n" );
	}
	else
	{
		*in_session = 0;
		(*active)->flag = false;
		pthread_mutex_unlock( &(*active)->mutex );
		*active = NULL;
		return sprintf( *message, "Successfully ended customer session \"%s\".\n", session_name );
	}
}

/* deposit takes a pointer to the bank, the amount to be deposited, a 
 * pointer to the message buffer, and a double pointer to the active
 * account. It simply adds amount to the active account's balance. If
 * successful, the function writes the 
 * appropriate message to the message buffer and returns the number of 
 * characters written. Otherwise, it returns -1.
 */
int
deposit( struct bank * Bank, float amount, char ** message, struct account ** active )
{
	if( !Bank )
	{
		return -1;
	}
	else if( !(*active) )
	{
		return sprintf( *message, "You do not currently have an active customer session.\n" );
	}
	else if( amount < 0 )
	{
		return sprintf( *message, "Nice try.\n" );
	}
	else
	{
		(*active)->balance += amount;
		return sprintf( *message, "Successfully deposited $%.2f to account \"%s\".\n", amount,
		(*active)->accountname );
	}
}

/* withdraw takes a pointer to the bank, the amount to be deposited, a 
 * pointer to the message buffer, and a double pointer to the active
 * account. It simply subtracts amount from the active account's balance.
 * If successful, the function writes the appropriate message to the
 * message buffer and returns the number of 
 * characters written. Otherwise, it returns -1.
 */
int
withdraw( struct bank * Bank, float amount, char ** message, struct account ** active )
{
	if( !Bank )
	{
		return -1;
	}
	else if( !(*active) )
	{
		return sprintf( *message, "You do not currently have an active customer session.\n" );
	}
	else if( amount < 0 )
	{
		return sprintf( *message, "You may not withdraw a negative amount.\n" );
	}
	else if( amount > (*active)->balance )
	{
		return sprintf( *message, "Insufficient funds.\n" );
	}
	else
	{
		(*active)->balance -= amount;
		return sprintf( *message, "Successfully withdrew $%.2f from account \"%s\".\n", amount,
		(*active)->accountname );
	}
}

/* query takes a pointer to the message buffer, the in_session flag, and
 * a pointer to the active account. It simply writes the appropriate
 * message to the message buffer and returns the number of characters
 * written.
 */
int
query( char ** message, int in_session, struct account * active )
{
	if( !in_session )
	{
		return sprintf( *message, "You do not currently have an active customer session.\n" );
	}
	else
	{
		return sprintf( *message, "Balance of account \"%s\": $%.2f\n", active->accountname,
		active->balance );
	}
}

/* quit takes the in_session flag, a pointer to the active account, and
 * the socket descriptor. If the client is currently in a customer 
 * session, the session is closed and the account's mutex is unlocked.
 * The function closes the socket and calls pthread_exit.
 */
void
quit( int in_session, struct account * active, int sd )
{
	if( in_session )
	{
		active->flag = false;
		pthread_mutex_unlock( &active->mutex );
	}
	
	close( sd );
	pthread_exit( 0 );
}
