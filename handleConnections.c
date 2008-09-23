#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

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
      struct sockaddr_storage remoteName;
      socklen_t sockAddrSize = sizeof(remoteName);
   
      modifyClientThreadCounter(1);
     				
      int clientSocket = accept(serverSocket, (struct sockaddr *) &remoteName,
				&sockAddrSize);
							
      if(clientSocket >= 0)
	{
	  pthread_t threadId;
	  pthread_attr_t threadAttr;
			 
	  pthread_attr_init(&threadAttr);
			 
	  if(pthread_attr_setdetachstate(&threadAttr,
					 PTHREAD_CREATE_DETACHED) == 0 &&
	     pthread_create(&threadId, &threadAttr,
			    &bridgeThread, (void *) &clientSocket) == 0)
	    {
	      pthread_attr_destroy(&threadAttr);
	    }
	  else
	    {
	      fprintf(stderr,
		      "unable to spawn new thread for %d",
		      clientSocket);
				
	      close(clientSocket);
	      modifyClientThreadCounter(-1);

	      rc = -1;
			 	
	      break;
	    }
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
  while(1)
    {
      FD_ZERO(&readFds);
      FD_SET(remoteSocket, &readFds);
      FD_SET(localSocket, &readFds);
	
      if(select(maxFd, &readFds, 0, 0, timeOut) <= 0)
	{
	  // timeout/error
	  break;
	}
		
      if(FD_ISSET(remoteSocket, &readFds) != 0)
	{
	  if(redirectData(remoteSocket,localSocket, readBuffer) <= 0)
	    {
	      rc = TRUE;
	      break;
	    }
	}
      if(FD_ISSET(localSocket, &readFds) != 0)
	{
	  if(redirectData(localSocket, remoteSocket, readBuffer) <= 0)
	    {
	      rc = TRUE;
	      break;
	    }
	}
    }
  close(localSocket);
  close(remoteSocket);
	
  return(rc);
}

void* bridgeThread(void* arg)
{
  int remoteSocket = *((int *) arg);
  int rc;
	
  if(0 != geteuid()) // run only as non-root
    {
      unsigned char* readBuffer = malloc(settings.bufferSize);
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
		
      if(rc >= 0)
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
	      if(rc != 0) // no timeout
		{
		  if(redirectData(remoteSocket, localSocket, readBuffer) <= 0)
		    {
		      close(localSocket);
		      close(remoteSocket);
		      free(readBuffer);

		      modifyClientThreadCounter(-1);

		      pthread_exit((void *) 0); // no receiver for return code
		    }
		}
		  
	      bridgeConnection(remoteSocket, localSocket,
			       readBuffer, connectionTimeout);
	    }
	  
	  if(addrInfoBase != NULL)
	    freeaddrinfo(addrInfoBase);
	}
      else
	{
	  close(remoteSocket);
	}
      free(readBuffer);
    }
  else
    {
      fprintf(stderr,
	      "%s() running only as non-root", __FUNCTION__);
    }

  modifyClientThreadCounter(-1);

  pthread_exit((void *) 0); // no receiver for return code
}

