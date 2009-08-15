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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <getopt.h>
#include <strings.h>
#include <string.h>
#include <pwd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <syslog.h>

#include "settings.h"
#include "utils.h"

const char* utilsId = "$Id$";

struct bufferList_t* pBufferListRoot = 0;
pthread_mutex_t bufferListMutex = PTHREAD_MUTEX_INITIALIZER;

extern struct configuration settings;

void resolvAddress (const char* hostname,
		    const char* port,
		    struct addrinfo** res)
{
  struct addrinfo hints;
  
  /* fill up hints */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
  hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
  hints.ai_protocol = 0;           /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  if(0 != getaddrinfo(hostname, port,
		      &hints, res))
  {
    fprintf(stderr, "getaddrinfo: failed\n");
  }
}	

int daemonize(const char* name)
{
	int rc = fork();
	extern struct configuration settings;
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
	int logFd = open("/var/log/csslh.log", O_RDWR|O_APPEND);
    dup2(zeroFd,STDIN_FILENO); /* stdin */
    dup2(logFd,STDOUT_FILENO); /* new stdout */ 
    dup2(logFd,STDERR_FILENO); /* new stderr */
    
    chdir(NEW_ROOT_DIR);
    
    // renice process
    nice(settings.niceLevel);        // lowest prio

    // prepare syslog
    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog("csslh",LOG_PID,LOG_DAEMON); 
    
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


int modifyClientThreadCounter(int delta)
{
  extern struct configuration settings;

  static volatile int clientCounter = 0;
  static pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
  static pthread_cond_t condition_cond  = PTHREAD_COND_INITIALIZER;
  
  
  pthread_mutex_lock(&count_mutex);

  if(delta > 0 && // increment
     clientCounter >= settings.maxClientThreads)
  {
	pthread_cond_wait(&condition_cond, &count_mutex);
  }
 
  clientCounter += delta;
 
  pthread_mutex_unlock(&count_mutex);
 
  if(delta < 0 &&
     clientCounter < settings.maxClientThreads)
  {
	pthread_cond_signal(&condition_cond);
  }

  return(clientCounter);
}

struct bufferList_t* allocBuffer(void)
{
  struct bufferList_t* pBufferListElement = 0;

  pthread_mutex_lock(&bufferListMutex);
  
  if(pBufferListRoot != 0)
  {
    pBufferListElement = pBufferListRoot;
    pBufferListRoot = pBufferListElement->next;
  }
  else
  {
    pBufferListElement = malloc(sizeof(struct bufferList_t));
    pBufferListElement->buffer = malloc(settings.bufferSize);
  }
  pthread_mutex_unlock(&bufferListMutex);
  
  return(pBufferListElement);
}

void freeBuffer(struct bufferList_t* pBufferListElement)
{
  pthread_mutex_lock(&bufferListMutex);

  pBufferListElement->next = pBufferListRoot;
  pBufferListRoot = pBufferListElement;

  pthread_mutex_unlock(&bufferListMutex);
}
  
