#include	<sys/types.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<pthread.h>
#include	"bank.h"

/* This is the server file to be submitted. */

struct bank *B;

int
claim_port( const char * port )
{
	struct addrinfo		addrinfo;
	struct addrinfo *	result;
	int			sd;
	char			message[256];
	int			on = 1;

	addrinfo.ai_flags = AI_PASSIVE;		/* for bind() */
	addrinfo.ai_family = AF_INET;		/* IPv4 only */
	addrinfo.ai_socktype = SOCK_STREAM;	/* Want TCP/IP */
	addrinfo.ai_protocol = 0;		/* Any protocol */
	addrinfo.ai_addrlen = 0;
	addrinfo.ai_addr = NULL;
	addrinfo.ai_canonname = NULL;
	addrinfo.ai_next = NULL;
	if ( getaddrinfo( 0, port, &addrinfo, &result ) != 0 )		/* want port 58288 */
	{
		fprintf( stderr, "\x1b[1;31mgetaddrinfo( %s ) failed errno is %s.  File %s line %d.\x1b[0m\n", port, strerror( errno ), __FILE__, __LINE__ );
		return -1;
	}
	else if ( errno = 0, (sd = socket( result->ai_family, result->ai_socktype, result->ai_protocol )) == -1 )
	{
		write( 1, message, sprintf( message, "socket() failed.  File %s line %d.\n", __FILE__, __LINE__ ) );
		freeaddrinfo( result );
		return -1;
	}
	else if ( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) == -1 )
	{
		write( 1, message, sprintf( message, "setsockopt() failed.  File %s line %d.\n", __FILE__, __LINE__ ) );
		freeaddrinfo( result );
		close( sd );
		return -1;
	}
	else if ( write( 1, message, sprintf( message, "\x1b[2;33mBinding to port %s ...\x1b[0m\n", port ) ), bind( sd, result->ai_addr, result->ai_addrlen ) == -1 )
	{
		freeaddrinfo( result );
		close( sd );
		write( 1, message, sprintf( message, "Bind to port %s failed. File %s line %d.\n", port, __FILE__, __LINE__ ) );
		return -1;
	}
	else
	{
		write( 1, message, sprintf( message,  "\x1b[1;32mSUCCESS : Bind to port %s\x1b[0m\n", port ) );
		freeaddrinfo( result );		
		return sd;			/* bind() succeeded; */
	}
}

void *
client_service_thread( void * arg )
{
	int				sd;
	char			request[2048];
	/* char			response[2048]; */
	/* char			temp; */
	/* int			i; */
	/* int			limit, size; */
	/* float			ignore;
	long			senderIPaddr; */
	char			cmd_buf[150];
	char			message[2048];
	char			buf2[101];
	char			session_name[101];
	int				in_session;
	int				quit_flag;
	char			*messPtr;
	char			*sNamePtr;
	char			*buf2ptr;
	double			amount;

	sd = *(int *)arg;
	free( arg );					/* keeping to memory management covenant */
	pthread_detach( pthread_self() );		/* Don't join on this thread */
	
	/*memcpy(cmd_buf, "\0", sizeof(cmd_buf));*/
	in_session = 0;
	quit_flag = 0;
	messPtr = &message[0];
	sNamePtr = &session_name[0];
	buf2ptr = &buf2[0];
	
	/* Keep taking input while socket is open and the user has not
	 * chosen to quit
	 */
	while ( read( sd, request, sizeof(request) ) > 0 && !quit_flag )
	{
		printf( "server receives input:  %s\n", request );
		
		memset(cmd_buf, '\0', sizeof(cmd_buf));
		memset(buf2, '\0', sizeof(buf2));
		sscanf(request, "%s %s", cmd_buf, buf2);
		
		if( strcmp( cmd_buf, "create" ) == 0 )
		{
			if( !(*buf2) )
			{
				sprintf( message, "You did not specify an account name to be created.\n" );
			}
			else if( (create_bank_account( B, buf2, in_session, &messPtr ) == -1 ) )
			{
				fprintf( stderr, "create_bank_account() croaked in %s line %d\n", __FILE__, __LINE__ );
				_exit( 1 );
			}
		}
		else if( strcmp( cmd_buf, "serve" ) == 0 )
		{
			if( !(*buf2) )
			{
				sprintf( message, "You did not specify an account name to be served.\n" );
			}
			else if( (serve_account( B, buf2, &in_session, &messPtr, &sNamePtr )) == -1 )
			{
				fprintf( stderr, "serve_account() croaked in %s line %d\n", __FILE__, __LINE__ );
				_exit( 1 );
			}
		}
		else if( strcmp( cmd_buf, "quit" ) == 0 )
		{
		}
		else if( strcmp( cmd_buf, "deposit" ) == 0 )
		{
			amount = atof(buf2ptr);
			if( (deposit( B, amount, &messPtr )) == -1 )
			{
				fprintf( stderr, "serve_account() croaked in %s line %d\n", __FILE__, __LINE__ );
				_exit( 1 );
			}
		}
		else if( strcmp( cmd_buf, "withdraw" ) == 0 )
		{
			amount = atof(buf2ptr);
			if( (withdraw( B, amount, &messPtr )) == -1 )
			{
				fprintf( stderr, "serve_account() croaked in %s line %d\n", __FILE__, __LINE__ );
				_exit( 1 );
			}
		}
		else if( strcmp( cmd_buf, "query" ) == 0 )
		{
		}
		else if( strcmp( cmd_buf, "end" ) == 0 )
		{
			if( (end( B, &in_session, &messPtr, sNamePtr )) == -1 )
			{
				fprintf( stderr, "serve_account() croaked in %s line %d\n", __FILE__, __LINE__ );
				_exit( 1 );
			}
		}
		else
		{
			if( !(*cmd_buf) )
			{
				sprintf( message, "No command given.\n" );
			}
			else
			{
				sprintf( message, "Command \"%s\" not recognized.\n", cmd_buf );
			}
		}
		
		write( sd, message, strlen(message) + 1 );
	}
	
	if( quit_flag )
	{
		close( sd );
	}
	return 0;
}

void *
session_accepter_thread( void * arg )
{
	socklen_t		ic;
	int				fd;
	int				sd;
	struct sockaddr_in      senderAddr;
	int 			*fdptr;
	pthread_t		cServiceTID;
	pthread_attr_t	attr;
	
	sd = *(int *)arg;
	free(arg);
	
	pthread_detach( pthread_self() );
	
	if ( pthread_attr_init( &attr ) != 0 )
	{
		printf( "pthread_attr_init() failed in file %s line %d\n", __FILE__, __LINE__ );
		_exit( 1 );
	}
	else if ( pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM ) != 0 )
	{
		printf( "pthread_attr_setscope() failed in file %s line %d\n", __FILE__, __LINE__ );
		_exit( 1 );
	}
	
	ic = sizeof(senderAddr);
	while ( (fd = accept( sd, (struct sockaddr *)&senderAddr, &ic )) != -1 )
	{
		if( (fdptr = (int *)malloc( sizeof(int) )) == 0)
		{
			printf( "malloc() failed in %s line %d.\n", __FILE__, __LINE__);
			_exit( 1 );
		}
		
		*fdptr = fd;					/* pointers are not the same size as ints any more. */
		
		if ( pthread_create( &cServiceTID, &attr, client_service_thread, fdptr ) != 0 )
		{
			printf( "pthread_create() failed in file %s line %d\n", __FILE__, __LINE__ );
			_exit( 1 );
		}
		else
		{
			continue;
		}
	}
	
	if ( pthread_attr_destroy( &attr ) != 0 )
	{
		printf( "pthread_attr_init() failed in file %s line %d\n", __FILE__, __LINE__ );
		_exit( 1 );
	}
	
	close( sd );
	return 0;
}

void *
periodic_action_cycle_thread( void * arg )
{
	return 0;
}

int
main( int argc, char ** argv )
{
	int			sd;
	int			error;
	char			message[256];
	pthread_t		sAccepterTID;
	pthread_t		pActionCycleTID;
	pthread_attr_t		kernel_attr;
	int				*sdptr;

	if ( pthread_attr_init( &kernel_attr ) != 0 )
	{
		printf( "pthread_attr_init() failed in file %s line %d\n", __FILE__, __LINE__ );
		return 0;
	}
	else if ( pthread_attr_setscope( &kernel_attr, PTHREAD_SCOPE_SYSTEM ) != 0 )
	{
		printf( "pthread_attr_setscope() failed in file %s line %d\n", __FILE__, __LINE__ );
		return 0;
	}
	else if ( (sd = claim_port( "58289" )) == -1 )
	{
		write( 1, message, sprintf( message,  "\x1b[1;31mCould not bind to port %s errno %s\x1b[0m\n", "58289", strerror( errno ) ) );
		return 1;
	}
	else if ( listen( sd, 100 ) == -1 )
	{
		printf( "listen() failed in file %s line %d\n", __FILE__, __LINE__ );
		close( sd );
		return 0;
	}
	else
	{
		if( (B = CreateBank()) == NULL)
		{
			printf( "CreateBank() failed in %s line %d.\n", __FILE__, __LINE__);
			return 1;
		}
		else if( (sdptr = (int *)malloc( sizeof(int) )) == 0)
		{
			printf( "malloc() failed in %s line %d.\n", __FILE__, __LINE__);
			return 1;
		}
		else if ( ( *sdptr = sd ), (error = pthread_create( &sAccepterTID, &kernel_attr, session_accepter_thread, sdptr )) != 0 )
		{
			printf( "pthread_create() croaked in %s line %d: %s\n",
			__FILE__, __LINE__, strerror( error ) );
			return 1;	/* crash and burn */
		}
		else if ( (error = pthread_create( &pActionCycleTID, &kernel_attr, periodic_action_cycle_thread, 0 )) != 0 )
		{
			printf( "pthread_create() croaked in %s line %d: %s\n",
			__FILE__, __LINE__, strerror( error ) );
			return 1;	/* crash and burn */
		}
	}
		
	if ( pthread_attr_destroy( &kernel_attr ) != 0 )
	{
		printf( "pthread_attr_init() failed in file %s line %d\n", __FILE__, __LINE__ );
		return 1;
	}
	pthread_exit( 0 );
}
