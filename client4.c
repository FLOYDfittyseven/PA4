#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<signal.h>
#include	<fcntl.h>
#include	<sys/time.h>
#include	<memory.h>
#include	<ifaddrs.h>
#include	<net/if.h>
#include	<stdarg.h>
#include	<sys/termios.h>
#include	<pthread.h>

int
connect_to_server( const char * server, const char * port )
{
	int			sd;
	struct addrinfo		addrinfo;
	struct addrinfo *	result;
	char			message[256];

	addrinfo.ai_flags = 0;
	addrinfo.ai_family = AF_INET;		/* IPv4 only */
	addrinfo.ai_socktype = SOCK_STREAM;	/* Want TCP/IP */
	addrinfo.ai_protocol = 0;		/* Any protocol */
	addrinfo.ai_addrlen = 0;
	addrinfo.ai_addr = NULL;
	addrinfo.ai_canonname = NULL;
	addrinfo.ai_next = NULL;
	if ( getaddrinfo( server, port, &addrinfo, &result ) != 0 )
	{
		fprintf( stderr, "\x1b[1;31mgetaddrinfo( %s ) failed.  File %s line %d.\x1b[0m\n", server, __FILE__, __LINE__ );
		return -1;
	}
	else if ( errno = 0, (sd = socket( result->ai_family, result->ai_socktype, result->ai_protocol )) == -1 )
	{
		fprintf( stderr, "\x1b[1;31msocket( %d, %d, %d ) failed. File %s line %d.\x1b[0m\n", result->ai_family, result->ai_socktype, 
		result->ai_protocol, __FILE__, __LINE__);
		freeaddrinfo( result );
		return -1;
	}
	else
	{
		do {
			if ( errno = 0, connect( sd, result->ai_addr, result->ai_addrlen ) == -1 )
			{
				sleep( 3 );
				write( 1, message, sprintf( message, "\x1b[2;33mConnecting to server %s ...\x1b[0m\n", server ) );
			}
			else
			{
				freeaddrinfo( result );
				return sd;		/* connect() succeeded */
			}
		} while ( errno == ECONNREFUSED );
		freeaddrinfo( result );
		return -1;
	}
}

void *
command_input_thread( void * p )
{
	int sd = *(int *)p;
	char			prompt[] = "Enter a command>>";
	char			string[512];
	int			len;
	void			*ret;
	
	pthread_detach( pthread_self() );
	
	while ( write( 1, prompt, sizeof(prompt) ), (len = read( 0, string, sizeof(string) )) > 0 )
		{
			string[len-1]= '\0';
			write( sd, string, strlen( string ) + 1 );
			sleep(2);
		}
		
		ret = NULL;
		pthread_exit( ret );
}

void *
response_output_thread( void * p )
{
	int 			sd;
	char			buffer[512];
	void			*ret;
	
	sd = *(int *)p;
	
	while ( (read( sd, buffer, sizeof(buffer) )) > 0 )
		{
			write( 1, buffer, strlen(buffer) );
			printf("\n");
		}
		
		ret = NULL;
		free(p);
		pthread_exit( ret );
}

int
main( int argc, char ** argv )
{
	int				sd;
	char			message[256];
	pthread_t		cInputTID;
	pthread_t		rOutputTID;
	pthread_attr_t	attr;
	int				*sptr;
	void 			*rptr;
	unsigned int	error;

	if ( argc < 2 )
	{
		fprintf( stderr, "\x1b[1;31mNo host name specified.  File %s line %d.\x1b[0m\n", __FILE__, __LINE__ );
		exit( 1 );
	}
	else if ( (sd = connect_to_server( argv[1], "58288" )) == -1 )
	{
		write( 1, message, sprintf( message,  "\x1b[1;31mCould not connect to server %s errno %s\x1b[0m\n", argv[1], strerror( errno ) ) );
		return 1;
	}
	else
	{
		printf( "Connected to server %s\n", argv[1] );
		
		/* BEGINNING OF THREAD USAGE: SETUP */
		if ( (sptr = (int *)malloc( sizeof(int) )) == 0 )
		{
			printf( "malloc() croaked in %s line %d: %s\n",
			__FILE__, __LINE__, strerror( errno ) );	/* BKR */
			_exit( 1 );	/* crash and burn */
		}
		else if ( *sptr = sd, (error = pthread_attr_init( &attr )) != 0 )
		{
			printf( "pthread_attr_init() croaked in %s line %d: %s\n",
			__FILE__, __LINE__, strerror( error ) );
			_exit( 1 );	/* crash and burn */
		}
		else if ( (error = pthread_attr_setscope( &attr, PTHREAD_SCOPE_SYSTEM )) != 0 )	/* BKR try PROCESS */
		{
			printf( "pthread_attr_setscope() croaked in %s line %d: %s\n",
			__FILE__, __LINE__, strerror( error ) );
			_exit( 1 );	/* crash and burn */
		}
		else if ( (error = pthread_create( &cInputTID, &attr, command_input_thread, sptr )) != 0 )
		{
			printf( "pthread_create() croaked in %s line %d: %s\n",
			__FILE__, __LINE__, strerror( error ) );
			_exit( 1 );	/* crash and burn */
		}
		else if ( (error = pthread_create( &rOutputTID, &attr, response_output_thread, sptr )) != 0 )
		{
			printf( "pthread_create() croaked in %s line %d: %s\n",
			__FILE__, __LINE__, strerror( error ) );
			_exit( 1 );	/* crash and burn */
		}
		else if ( (error = pthread_join( rOutputTID, (void **)&rptr )) != 0 )
		{
			printf( "pthread_join() croaked in %s line %d: %s\n",
			__FILE__, __LINE__, strerror( error ) );
			_exit( 1 );	/* crash and burn */
		}
		else
		{
			printf("Quitting client process.\n");
		}
		/* END OF THREAD USAGE: JOINING */
		
		
		close( sd );
		pthread_attr_destroy( &attr );
		_exit( 0 );
	}
}

