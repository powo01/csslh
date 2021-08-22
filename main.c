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
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <syslog.h>

#include "config.h"
#include "utils.h"
#include "handleConnections.h"

int main(int argc, char* argv[])
{
	int rc = parseCommandLine(argc, argv);
	
	// resolv port to numeric
	int publicPort;
	
	// port in digits or service name
	if(isdigit(*(pGetConfig()->publicPort)))
	{
		char* ptr = pGetConfig()->publicPort;
		
		// test for all digits
		while(isdigit(*(++ptr)));
		
		if(*ptr)
		{
			fprintf(stderr,
					"error: Portnumber contains an non digit\n");
			return(EINVAL);
		}
		else
		{
			// get port number from string
			publicPort = atoi(pGetConfig()->publicPort);
		
			// port number in valid range ?
			if(publicPort < 1 || publicPort > 65535)
			{
				fprintf(stderr,
					"error: Portnumber %d out of range\n",
					publicPort);
				return(EINVAL);
			}
		}
	}
	else
	{
		// resolve service
		struct servent* resolvPort = getservbyname(pGetConfig()->publicPort, "tcp");
		
		if(resolvPort == NULL)
		{
			fprintf(stderr,
				"error: unable to map tcp service %s to port number\n",
				pGetConfig()->publicPort);
				
			return(EINVAL);
		}
		
		publicPort = ntohs(resolvPort->s_port);
	
		syslog(LOG_DEBUG, "public port = %s -> %d\n", pGetConfig()->publicPort, publicPort);
		
		endservent();
	}
	
	// test for root
	if(0 == geteuid() || // need to be root
	   publicPort >= 1024)
	{   
	  struct addrinfo* addrInfo;
	  struct addrinfo* addrInfoBase;
	  int serverSockets[maxServerBindAddresses];
	  int socketIndex = 0;

	  resolvAddress(pGetConfig()->publicHostname, pGetConfig()->publicPort,
			&addrInfoBase);
	  
          addrInfo = addrInfoBase;

	  while(addrInfo != NULL)
	    {
	      if(socketIndex == maxServerBindAddresses)
	      {
		fprintf(stderr,
                          "error: more than %d server bind addresses,\n", maxServerBindAddresses);
		return(EINVAL);
	      }
	      int serverSocket = socket(addrInfo->ai_family,
				    addrInfo->ai_socktype,
				    addrInfo->ai_protocol);
	      if(serverSocket >= 0)
	      {
	      	if(bind(serverSocket, addrInfo->ai_addr, addrInfo->ai_addrlen) != -1 &&
		   listen(serverSocket, pGetConfig()->maxClientThreads) != -1)
		   {
			serverSockets[socketIndex++] = serverSocket;
		   }
	     	   else
		   {  
	      		close(serverSocket);
		   }
	      }

              addrInfo = addrInfo->ai_next;
            }

	     if(addrInfoBase != NULL)
	     {
                freeaddrinfo(addrInfoBase);
		addrInfoBase = NULL;
	     }

	      if(socketIndex > 0 && 
		 daemonize(argv[0]) != -1)	
		{
		#ifdef BUILD_VERSION
		  extern const char* buildVersion;
		  syslog(LOG_NOTICE,"%s started, BuildVersion %s\n",argv[0], buildVersion);
		#endif
		  initBuffer();
		  handleConnections(serverSockets,socketIndex);
		}
	      else
		{
		  fprintf(stderr,
			  "error bind/listen, errno=%d\n",errno);
		}
	      
	} // serverSocket
	else
	  {
		fprintf(stderr,"%s must be started as root to use port %s\n",
			argv[0], pGetConfig()->publicPort);
		
		rc = EACCES;
	  }
	return(rc);
}
