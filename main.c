#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>

#include "settings.h"
#include "utils.h"
#include "handleConnections.h"

const char* versionId = "$Id$";

extern struct configuration settings;

int main(int argc, char* argv[])
{
	int rc = parseCommandLine(argc, argv);
	
	// test for root
	if(0 == geteuid()) // need to be root
	{ 
		// get https port on internet side
		int serverSocket = socket(PF_INET,SOCK_STREAM, 0);
		
		if(serverSocket >= 0)
		{
			struct sockaddr_in servername;
			init_sockaddr(&servername, settings.publicHostname, settings.publicPort);
			
			if(bind(serverSocket, (struct sockaddr *) &servername, sizeof(servername)) >= 0 &&
			   daemonize(argv[0]) >= 0 &&	
			   listen(serverSocket, 5) >= 0)	
			{
				handleConnections(serverSocket);
			} // bind
			else
			{
				fprintf(stderr,
						"error bind/listen");
			}
		} // serverSocket
	}
	else
	{
		fprintf(stderr,"%s must be started as root\n\r",
					argv[0]);
					
		rc = -1;
	}
	return(rc);
}
