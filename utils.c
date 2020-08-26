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

#include "config.h"
#include "utils.h"

const char* utilsId = "$Id$";

#define NEW_ROOT_DIR "/tmp"

struct bufferList_t* pBufferListRoot = 0;
pthread_mutex_t bufferListMutex = PTHREAD_MUTEX_INITIALIZER;
volatile int clientCounter = 0;

pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_cond  = PTHREAD_COND_INITIALIZER;
volatile int lockedThreads = 0;


void resolvAddress (const char* hostname,
		    const char* port,
		    struct addrinfo** res)
{
  struct addrinfo hints;
  
  /* fill up hints */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = PF_UNSPEC;     /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
  hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
  hints.ai_protocol = 0;           /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  int exitCode = getaddrinfo(hostname, port,
		      &hints, res);

  if(exitCode != EXIT_SUCCESS)
  {
	syslog(LOG_ERR,"%s: %s (errno=%d)", __FUNCTION__, strerror(exitCode), exitCode);
	
    	exit(exitCode);
  }
}	

int daemonize(const char* name)
{
	int rc = fork();
	if(rc > 0)
	{
		fprintf(stderr,"%s started ... (pid=%d)\n\r",
				name, rc);

		FILE* pidFd = fopen("/var/run/csslh.pid", "w");
		
		if(pidFd != 0)
		{
			fprintf(pidFd, "%d", rc);
			fclose(pidFd);
		}
				
		exit(0); // server quit
	}
	if(rc < 0)
	{
		exit(1); // error
	}
	
	signal(SIGHUP,SIG_IGN); // ignore HUP signal
	signal(SIGPIPE, SIG_IGN);  // ignore "Brocken Pipe" signal
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
    nice(pGetConfig()->niceLevel);        // lowest prio

    // prepare syslog
    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog("csslh",LOG_PID,LOG_DAEMON); 
    
    if(0 == geteuid()) // only for root
    {
    	struct passwd* userSystemData = getpwnam(pGetConfig()->username);
    
        // leave user and group root	
    	if(userSystemData == 0 ||
	   setegid(userSystemData->pw_gid) != 0 ||
	   seteuid(userSystemData->pw_uid) !=0 )
    	{
    		fprintf(stderr,"unable to change user and group");
    		exit(1);
    	}
    }
    return(rc);
}


int modifyClientThreadCounter(int delta)
{
  int counterValue = 0;
  
  pthread_mutex_lock(&count_mutex);

  if(delta > 0) // increment
  {
	clientCounter++;

	if(clientCounter > pGetConfig()->maxClientThreads)
	{
		lockedThreads++;
		pthread_cond_wait(&condition_cond, &count_mutex);
	}
  } else if(delta < 0) //decrement
  {
	clientCounter--;

	if(clientCounter <= pGetConfig()->maxClientThreads)
	{
		if(lockedThreads > 0)
		{
			lockedThreads--;
			pthread_cond_signal(&condition_cond);
		}
	}
  }

  counterValue = clientCounter;

  pthread_mutex_unlock(&count_mutex);

  return(counterValue);
}

void initBuffer(void)
{
	pthread_mutex_lock(&bufferListMutex);
	
	if(pBufferListRoot == NULL)
  {
    int elements = pGetConfig()->maxClientThreads;
  
    pBufferListRoot = malloc(elements * sizeof(struct bufferList_t));

    if(pBufferListRoot != NULL)
    {
      struct bufferList_t* listPtr = pBufferListRoot;

      const size_t bufferSize = pGetConfig()->bufferSize;
      unsigned char* bufferPtr = malloc(elements * bufferSize);

      if(bufferPtr != NULL)
      {
        while(elements)
        {
          listPtr->buffer = bufferPtr;
          bufferPtr += bufferSize;

          // other than last element
          if(--elements)
          {
            struct bufferList_t* nextPtr = listPtr;
            listPtr->next = ++nextPtr;
            listPtr = nextPtr;
          }
          else
          {
            listPtr->next = NULL;
          }
        }
      }
      else
      {
        free(pBufferListRoot);
        pBufferListRoot = NULL;
      }
    }
  }
  
  pthread_mutex_unlock(&bufferListMutex);
}

struct bufferList_t* allocBuffer(void)
{
  struct bufferList_t* pBufferListElement = 0;

  pthread_mutex_lock(&bufferListMutex);
  
  if(pBufferListRoot != 0)
  {
    pBufferListElement = pBufferListRoot;
    pBufferListRoot = pBufferListElement->next;

    if(pBufferListElement->next != 0)
  	pBufferListElement->next = 0; 
  }

  pthread_mutex_unlock(&bufferListMutex);

  return(pBufferListElement);
}

void freeBuffer(struct bufferList_t* pBufferListElement)
{
  if(pBufferListElement != 0 &&
     pBufferListElement->buffer != 0 )
  {
    pthread_mutex_lock(&bufferListMutex);

    // empty list
    if(pBufferListRoot == NULL)
    {
      pBufferListRoot = pBufferListElement;
    }
    else
    {
      struct bufferList_t* listPtr = pBufferListRoot;

      // find last element and append
      while(listPtr->next != NULL)
      {
        listPtr = listPtr->next;
      }

      listPtr->next = pBufferListElement;
    }
    
    pthread_mutex_unlock(&bufferListMutex);
  }
  else
	syslog(LOG_ERR,"unable to free empty list element");
}
  
