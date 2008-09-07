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


#include "settings.h"
#include "utils.h"

const char* utilsId = "$Id$";

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

