#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>

#include "utils.h"
#include "settings.h"
#include "readWrite.h"
#include "handleConnections.h"

const char* handleConnectionsId = "$Id$";

extern struct configuration settings;

pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  condition_cond  = PTHREAD_COND_INITIALIZER;

int modifyClientThreadCounter(int delta)
{
	int rc = FALSE;
	int maxClientThreads = 8;

	static clientCounter = 0;
	static pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&count_mutex);
	if((delta > 0 && clientCounter < maxClientThreads) ||
	    delta <= 0)
	{
		clientCounter += delta;
		rc = TRUE;
	}
	pthread_mutex_unlock(&count_mutex);

	return(rc);
}
	
int handleConnections(int serverSocket)
{
	int rc = 0;
	
	while(1)
	{
		struct sockaddr_in remoteName;
		socklen_t sockAddrSize = sizeof(remoteName);
		int clientThreadReady;

		pthread_mutex_lock(&condition_mutex);
		clientThreadReady = modifyClientThreadCounter(1);
		while(FALSE == clientThreadReady)
		{
			pthread_cond_wait( &condition_cond, &condition_mutex );
			clientThreadReady = modifyClientThreadCounter(1);
		}
		pthread_mutex_unlock(&condition_mutex);
				
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
			struct sockaddr_in servername;
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
			
			init_sockaddr (&servername, localHost, localPort);
			localSocket = socket(PF_INET,SOCK_STREAM, 0);
			
			if(localSocket >= 0 &&
			   connect(localSocket,
			   			(struct sockaddr *) &servername,
	                    sizeof (servername)) == 0)
	        {
	        	if(rc != 0) // no timeout
	        	{
	    			if(redirectData(remoteSocket, localSocket, readBuffer) <= 0)
	    			{
	    				close(localSocket);
	    				close(remoteSocket);
	    				free(readBuffer);
	    				pthread_exit((void *) 0); // no receiver for return code
	    			}
	        	}
	        	
	        	bridgeConnection(remoteSocket, localSocket,
	        					 readBuffer, connectionTimeout);
	        } 			    			
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

	pthread_mutex_lock( &condition_mutex );
	if(TRUE == modifyClientThreadCounter(-1)) // unregister client thread
	{
		pthread_cond_signal( &condition_cond );
	}
	pthread_mutex_unlock(&condition_mutex );

	pthread_exit((void *) 0); // no receiver for return code
}

