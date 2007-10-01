#ifndef UTILS_H_
#define UTILS_H_

void init_sockaddr (struct sockaddr_in *name,
     	            const char* hostname,
        		    const char* port);
int daemonize(const char* name);
 
#endif /*UTILS_H_*/
