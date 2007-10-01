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

