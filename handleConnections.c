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
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <syslog.h>
#include <errno.h>

#include "utils.h"
#include "settings.h"
#include "readWrite.h"
#include "handleConnections.h"

const char* handleConnectionsId = "$Id$";

extern struct configuration settings;
	
int handleConnections(int serverSocket)
{
  int rc = 0;
	
  while(1)
    {
      struct sockaddr_storage remoteClient;
      socklen_t sockAddrSize = sizeof(struct sockaddr_storage);
        				
      int clientSocket = accept(serverSocket, (struct sockaddr *) &remoteClient,
				&sockAddrSize);
							
      if(clientSocket != -1)
	{
          modifyClientThreadCounter(1);

	  pthread_t threadId;
	  pthread_attr_t threadAttr;
	
	  pthread_attr_init(&threadAttr);
	  pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
	  
	  if(pthread_create(&threadId, &threadAttr,
			    &bridgeThread, (void *) &clientSocket) == 0)
	    {
	      syslog(LOG_DEBUG,
		     "spawn new thread for %d",
                      	clientSocket);
	    }
	  else
	    {
	      // fprintf(stderr,
	      syslog(LOG_ERR,
		      "unable to spawn new thread for %d",
		      clientSocket);
				
	      close(clientSocket);
	      modifyClientThreadCounter(-1);
	    }
	    
	    pthread_attr_destroy(&threadAttr);
	    
	} 
	else // if clientSocket
	{
		syslog(LOG_ERR,
		       "unable to accept client, errno = %d",
			errno);
	}
		
    } // while

  return(rc);
}

int bridgeConnection(int remoteSocket, int localSocket,
		     unsigned char* readBuffer, struct timeval* timeOut)
{
  int rc = FALSE;
  fd_set readFds;
  const int maxFd = (remoteSocket > localSocket ?
		     remoteSocket : localSocket) + 1;

  ssize_t ingressCounter = 0;
  ssize_t egressCounter = 0;

  while(FALSE == rc)
    {
      FD_ZERO(&readFds);
      FD_SET(remoteSocket, &readFds);
      FD_SET(localSocket, &readFds);
	
      int rtn = select(maxFd, &readFds, 0, 0, timeOut);

      if(rtn == -1 || rtn == 0)
	{
	  // timeout/error
	  rc = TRUE;
	  break;
	}
		
      if(FD_ISSET(remoteSocket, &readFds) != 0)
	{
	  ssize_t bytes = redirectData(remoteSocket,localSocket, readBuffer);

	  if(bytes <= 0)
	    {
	      rc = TRUE;
	    }
	  else
	    {
	      ingressCounter += bytes;
	    }
	}
      if(FD_ISSET(localSocket, &readFds) != 0)
	{
	  ssize_t bytes = redirectData(localSocket, remoteSocket, readBuffer);
	  if(bytes <= 0)
	    {
	      rc = TRUE;
	    }
	  else
	    {
	      egressCounter += bytes;
	    }
	}
    }
  
  syslog(LOG_INFO,
	 "%s(): ingressCounter = %ld, egressCounter = %ld",
	 __FUNCTION__, ingressCounter, egressCounter);

  return(rc);
}

void* bridgeThread(void* arg)
{
  int remoteSocket = *((int *) arg);
  int rc;
	
  if(0 != geteuid()) // run only as non-root
    {
      char* localPort = settings.sslPort;
      char* localHost = settings.sslHostname;
      struct timeval sshDetectTimeout = { settings.timeOut , 0 }; // 2 sec
      struct timeval sslConnectionTimeout = { 120, 0 }; // 120 sec
      struct timeval* connectionTimeout = 0;
      fd_set readFds;
		
      // test for ssl/ssh connection
      FD_ZERO(&readFds);
      FD_SET(remoteSocket, &readFds);
      rc = select(remoteSocket+1, &readFds, 0, 0, &sshDetectTimeout);
		
      if(rc != -1)
	{
	  struct addrinfo* addrInfo;
	  struct addrinfo* addrInfoBase;
	  int localSocket;
			
	  if(rc == 0) // timeout -> ssh connection
	    {
	      localPort = settings.sshPort;
	      localHost = settings.sshHostname;
	    }
	  else	// ssl connection
	    {
	      connectionTimeout = &sslConnectionTimeout;
	    }
		  
	  resolvAddress(localHost, localPort, &addrInfo);

	  for(addrInfoBase = addrInfo; // need for delete
	      addrInfo != NULL; addrInfo = addrInfo->ai_next)
	    {
	      localSocket = socket(addrInfo->ai_family,
				   addrInfo->ai_socktype,
				   addrInfo->ai_protocol);
			
	      if(localSocket == -1)
		{
		  continue;
		}

	      if(connect(localSocket,
			 addrInfo->ai_addr,
			 addrInfo->ai_addrlen) != -1)
		break;
	      

	      close(localSocket);
	    }

	  if (addrInfo != NULL)               /* address succeeded */
	    {
              struct bufferList_t* pBufferListElement = allocBuffer();
      
	      if(rc != 0) // no timeout
		{
		  if(redirectData(remoteSocket, localSocket,
			 pBufferListElement->buffer) > 0)
		    {
		      rc = 0; // handle as normal connection
		    }
		}
		
	      if(rc == 0)
		{  
	      		bridgeConnection(remoteSocket, localSocket,
				       pBufferListElement->buffer, connectionTimeout);
                }

	      close(localSocket);
              
	      freeBuffer(pBufferListElement);
	    }
	  
	  if(addrInfoBase != NULL)
	    freeaddrinfo(addrInfoBase);
	}
    }
  else
    {
      syslog(LOG_ERR,
	      "%s() threads running only as non-root", __FUNCTION__);
    }

 
  close(remoteSocket);
  modifyClientThreadCounter(-1);

  return NULL; // no receiver for return code
}

