#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <strings.h>
#include <string.h>

#include "settings.h"


struct configuration settings;

void splitHostPort(char* hostPort, char** hostPart, char** portPart)
{
	// input format hostname:port
	char* ptr = index(hostPort,':');
	if(ptr != 0)
	{
		*ptr++ = '\0';
		*hostPart = malloc(strlen(hostPort)+1);
		strcpy(*hostPart, hostPort);
	}
	else
	{
		ptr = hostPort;
	}
	
	*portPart = malloc(strlen(ptr)+1);
	strcpy(*portPart, ptr);
}
 
int parseCommandLine(int argc, char* argv[])
{
	int rc = TRUE;
	extern const char* versionId;
	
	const struct option long_options[] =
	{
		{ "port", required_argument, 0, 'p' },
		{ "ssh", required_argument, 0, 's' },
		{ "ssl", required_argument, 0, 'l' },
		{ "timeout", required_argument, 0, 't' },
		{ "buffersize", required_argument, 0, 'b' },
		{ "nicelevel", required_argument, 0, 'n' },
		{ "help", no_argument, 0, 'h' },
		{ "version", no_argument, 0, 'v' },
		{ "limitThreads", required_argument, 0, 'x'},
		{ 0, 0, 0, 0  }
	};
	
	/* set default values */
	settings.publicHostname = settings.sshHostname = settings.sslHostname = "localhost";
	settings.publicPort = settings.sslPort = "https";
	settings.sshPort = "ssh";
	settings.timeOut = 2;
	settings.bufferSize = BUFFERSIZE;
	settings.niceLevel = 19;
	settings.username = "nobody";
	settings.maxClientThreads = 7;
	
	while (optind < argc)
	{
		int index = -1;
		int result = getopt_long(argc, argv, "p:s:l:t:b:n:hvx:",
				long_options, &index);
		if (result == -1)
			break; /* end of list */
		switch (result)
		{
			case 'v': /* print version */
				fprintf(stderr,"Version: %s\n\r", versionId);
				exit(0);
				break;
			case 'p': /* same as index==0 */
				splitHostPort(optarg,
							  &settings.publicHostname, &settings.publicPort);
				break;
			case 's': /* same as index==1 */
				splitHostPort(optarg,	
						&settings.sshHostname, &settings.sshPort);				
				break;
			case 'l': /* same as index==2 */
				splitHostPort(optarg,		
						&settings.sslHostname, &settings.sslPort);							
				break;
			case 't': /* same as index==3 */
				settings.timeOut = atoi(optarg);
				break;
			case 'b': /* same as index==4 */
				settings.bufferSize = atoi(optarg);
				break;
			case 'n': /* same as index==4 */
				settings.niceLevel = atoi(optarg);
				break;
			case 'x':
				settings.maxClientThreads = atoi(optarg);
				break;				
			default: /* unknown */
				break;
		}
	}
	/* print all other parameters */
	while (optind < argc)
	{
		fprintf(stderr,
				"other parameter: <%s>\n", argv[optind++]);
	}

	return(rc);
}

