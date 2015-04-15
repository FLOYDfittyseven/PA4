#include "bank.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
	
	memcpy(ret->accountname, "\0", sizeof(ret->accountname));
	memcpy(ret->accountname, accountname, sizeof(accountname));
	
	ret->balance = 0;
	ret->flag = false;
	
	return ret;
}

void
DestroyAccount( struct account * a )
{
	if( !a )
	{
		return;
	}
	
	free( a );
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
create_bank_account( struct account ** bank, char * accountname, int in_session, char ** message )
{
	int i;
	
	if( !bank | !accountname )
	{
		return -1;
	}
	else if( in_session )
	{
		return sprintf( *message, "You may not create accounts while in a customer session.\n");
	}
	
	for( i=0; i<20; i++ )
	{
		if( !bank[i] )
		{
			break;
		}
		else if( strcmp( accountname, bank[i]->accountname ) == 0 )
		{
			return sprintf( *message, "Account \"%s\" already exists.\n", accountname );
		}
		else
		{
			continue;
		}
	}
	
	if( i == 20)
	{
		return sprintf( *message, "Bank is full.\n" );
	}
	
	bank[i] = CreateAccount( accountname );
	return sprintf( *message, "Account \"%s\" successfully created.\n", accountname);;
}

/*
int serve_account( struct account * bank, char * accountname )
{
}

int deposit( struct account * bank, char * accountname, float amount )
{
}

int withdraw( struct account * bank, char * accountname, float amount )
{
}

float query( struct account * bank, char * accountname )
{
}

int end( struct account * bank, char * accountname )
{
}*/

