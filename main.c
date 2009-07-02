#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

#include "settings.h"
#include "utils.h"
#include "handleConnections.h"

const char* versionId = "$Id: main.c 106 2008-09-07 16:26:09Z wolfgang $";

extern struct configuration settings;

int main(int argc, char* argv[])
{
	int rc = parseCommandLine(argc, argv);
	
	// test for root
	if(0 == geteuid()) // need to be root
	{ 
	  
	  struct addrinfo* addrInfo;
	  struct addrinfo* addrInfoBase;
	  int serverSocket;

	  resolvAddress(settings.publicHostname, settings.publicPort,
			&addrInfo);
	  
	  for(addrInfoBase = addrInfo;
	      addrInfo != NULL; addrInfo->ai_next)
	    {
	      serverSocket = socket(addrInfo->ai_family,
				    addrInfo->ai_socktype,
				    addrInfo->ai_protocol);
	      if(serverSocket == -1)
		continue;

	      if(bind(serverSocket,
		      addrInfo->ai_addr, addrInfo->ai_addrlen) == 0)
		break;

	      close(serverSocket);
	    }

	      if(addrInfo != NULL &&
		 daemonize(argv[0]) >= 0 &&	
		 listen(serverSocket, 5) >= 0)	
		{
		  handleConnections(serverSocket);
		}
	      else
		{
		  fprintf(stderr,
			  "error bind/listen");
		}
	      if(addrInfoBase != NULL)
		freeaddrinfo(addrInfoBase);
	} // serverSocket
	else
	  {
		fprintf(stderr,"%s must be started as root\n\r",
			argv[0]);
		
		rc = -1;
	  }
	return(rc);
}
