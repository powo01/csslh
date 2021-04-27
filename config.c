/*
Copyright Wolfgang Poppelreuter <support.csslh@poppelreuter.de>

This file is part of csslh.

csslh is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
any later version.

csslh is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with csslh.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <strings.h>
#include <string.h>

#include "config.h"

#define BUFFERSIZE 2048

#ifdef BUILD_VERSION
const char* buildVersion = BUILD_VERSION;
#endif

config_t settings;

config_t const*  pGetConfig(void)
{
	return &settings;
}

void splitHostPort(char* hostPort, char** hostPart, char** portPart)
{
	// input format hostname:port
	char* ptr = strrchr(hostPort,':'); // find last delimiter
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

	const struct option long_options[] =
	{
		{ "port", required_argument, 0, 'p' },
		{ "ssh", required_argument, 0, 's' },
		{ "ssl", required_argument, 0, 'l' },
		{ "timeout", required_argument, 0, 't' },
		{ "buffersize", required_argument, 0, 'b' },
		{ "nicelevel", required_argument, 0, 'n' },
		{ "limitThreads", required_argument, 0, 'x'},
		{ "PidFile", required_argument, 0, 'P'},
#ifdef BUILD_VERSION
		{ "version", no_argument, 0, 'v' },
#endif
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
	settings.pidFile = NULL;
	
	while (optind < argc)
	{
		int index = -1;
		int result = getopt_long(argc, argv, "p:s:l:t:b:n:x:P:v",
				long_options, &index);
		if (result == -1)
			break; /* end of list */
		switch (result)
		{
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
			case 'n': // index==5
				settings.niceLevel = atoi(optarg);
				break;
			case 'x': // index==6
				settings.maxClientThreads = atoi(optarg);
				break;
			case 'P': // index==7
				settings.pidFile = malloc(strlen(optarg)+1);
				strcpy(settings.pidFile, optarg);
				break;
#ifdef BUILD_VERSION
			case 'v': /* index==8, print version */
				fprintf(stderr,"Version: %s\n\r", buildVersion);
				exit(0);
				break;
#endif
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
