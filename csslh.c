#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <strings.h>
#include <string.h>
#include <pwd.h>
#include <ctype.h>

#include "settings.h"
#include "readWrite.h"
#include "csslh.h"

const char* versionId = "$Id$";

extern struct configuration settings;

void init_sockaddr (struct sockaddr_in *name,
     	            const char *hostname,
        		    const char* port)
{
	struct hostent *hostinfo;

	name->sin_family = AF_INET;
	if(isdigit(*port))
	{
		name->sin_port = htons (atoi(port));
	}
	else
	{
		struct servent* portInfo = getservbyname(port,"tcp");
		
		if(portInfo)
		{
			name->sin_port = portInfo->s_port;
		}
		else
		{
			fprintf(stderr,"%s(): unable to resolve service %s to port number\n\r",
					__FUNCTION__,port);
			exit(1);
		}
	}
    hostinfo = gethostbyname (hostname);
    if (hostinfo == NULL)
    {
		fprintf (stderr, "Unknown host %s.\n", hostname);
    		        return;
    }
   	name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
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
	pthread_exit((void *) 0); // no receiver for return code
}

int daemonize(const char* name)
{
	int rc = fork();
	
	if(rc > 0)
	{
		fprintf(stderr,"%s started ... (pid=%d)\n\r",
				name, rc);
				
		exit(0); // server quit
	}
	if(rc < 0)
	{
		exit(1); // error
	}
	
	signal(SIGHUP,SIG_IGN); // ignore HUP signal
	setsid(); /* obtain a new process group */
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);
	int zeroFd = open("/dev/zero",O_RDWR);
//	snprintf(logFile, sizeof(logFile) -1 ,
//			"/var/log/%s.log", name); 
//	int logFd = open(logFile, O_RDWR|O_APPEND);
	int logFd = open("/var/log/csslh.log", O_RDWR|O_APPEND);
    dup2(zeroFd,STDIN_FILENO); /* stdin */
    dup2(logFd,STDOUT_FILENO); /* new stdout */ 
    dup2(logFd,STDERR_FILENO); /* new stderr */
    
    chdir(NEW_ROOT_DIR);
    
    // renice process
    nice(settings.niceLevel);        // lowest prio
 
    
    if(0 == geteuid()) // only for root
    {
    	struct passwd* userSystemData = getpwnam(settings.username);
    	
    	if(userSystemData != 0)
    	{
    		// leave user and group root
    		setegid(userSystemData->pw_gid);
    		seteuid(userSystemData->pw_uid);
    	}
    	else
    	{
    		fprintf(stderr,"unable to change user and group");
    		exit(1);
    	}
    }
	return(rc);
}

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
				while(1)
				{
					struct sockaddr_in remoteName;
					socklen_t sockAddrSize = sizeof(remoteName);
					
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
