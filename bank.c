#include "bank.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

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
	b->active = NULL;
	
	return b;
}


void
DestroyBank( struct bank * b )
{
	
	while( b->numAccounts > 0 )
	{
		DestroyAccount( b->accounts[b->numAccounts] );
		b->numAccounts--;
	}
	free( b->accounts );
	free( b );
	return;
}

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
 * create_bank_account takes a pointer to the array of accounts (the bank)
 * and the name of the account to be created. If the function fails, it 
 * returns -1. If the bank is not full and the account does not already exist,
 * the function creates a new account and adds it to the next
 * available spot in the bank. The function sets the appropriate message
 * and returns a the number of characters in the message (not including the 
 * trailing null byte).
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


int
serve_account( struct bank * Bank, char * accountname, int * in_session, char ** message, char ** session_name )
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
	else if( (pthread_mutex_unlock( &Bank->mutex )), (pthread_mutex_lock( &ptr->mutex )) == 0 )
	{
		*in_session = 1;
		strcpy( *session_name, accountname );
		ptr->flag = true;
		Bank->active = ptr;
		return sprintf( *message, "Now serving account \"%s\".\n", accountname );
	}
	
	return -1;
}

int
end( struct bank * Bank, int * in_session, char ** message, char * session_name )
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
		Bank->active->flag = false;
		pthread_mutex_unlock( &Bank->active->mutex );
		Bank->active = NULL;
		return sprintf( *message, "Successfully ended customer session \"%s\".\n", session_name );
	}
}

int
deposit( struct bank * Bank, float amount, char ** message )
{
	if( !Bank )
	{
		return -1;
	}
	else if( !(Bank->active) )
	{
		return sprintf( *message, "You do not currently have an active customer session.\n" );
	}
	else if( amount < 0 )
	{
		return sprintf( *message, "Nice try.\n" );
	}
	else
	{
		Bank->active->balance += amount;
		return sprintf( *message, "Successfully deposited $%.2f to account \"%s\".\n", amount,
		Bank->active->accountname );
	}
}

int
withdraw( struct bank * Bank, float amount, char ** message )
{
	if( !Bank )
	{
		return -1;
	}
	else if( !(Bank->active) )
	{
		return sprintf( *message, "You do not currently have an active customer session.\n" );
	}
	else if( amount < 0 )
	{
		return sprintf( *message, "You may not withdraw a negative amount.\n" );
	}
	else if( amount > Bank->active->balance )
	{
		return sprintf( *message, "Insufficient funds.\n" );
	}
	else
	{
		Bank->active->balance -= amount;
		return sprintf( *message, "Successfully withdrew $%.2f from account \"%s\".\n", amount,
		Bank->active->accountname );
	}
}

/*
float query( struct account * bank, char * accountname )
{
}

int
quit()
{
}*/
