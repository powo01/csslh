#ifndef CSSLH_H_
#define CSSLH_H_

void init_sockaddr (struct sockaddr_in *name,
     	            const char* hostname,
        		    const char* port);
int bridgeConnection(int remoteSocket, int localSocket,
					 unsigned char* readBuffer, struct timeval* timeOut);
void* bridgeThread(void* arg);
int daemonize(const char* name);
       		  
#endif /*CSSLH_H_*/
