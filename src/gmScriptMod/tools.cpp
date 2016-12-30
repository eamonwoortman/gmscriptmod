// Tools cpp file
#include <q_shared.h>
#include <g_local.h>
#include <qmmapi.h>

/*
==================
ConcatArgs

Dutchmeat:
This method allows us to get multiple arguments
Example, if the server command is:  'say hello all this is neelix bot'
then you use ConcatArgs(1); why 1? you start at 1, because our first argument is hello and not say.

==================
*/
char	*ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];
	
	len = 0;
	c = g_syscall( G_ARGC );

	for ( i = start ; i < c ; i++ ) {

		g_syscall(G_ARGV, i, arg, sizeof(arg));
		

		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

