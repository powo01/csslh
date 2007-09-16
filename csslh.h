#ifndef CSSLH_H_
#define CSSLH_H_

#define TRUE (1==1)
#define FALSE (2==1)

#define SSH_PORT 22
#define SSL_PORT 443
#define NEW_ROOT_DIR "/tmp"

#define BUFFERSIZE 2048

struct configuration
{
	char* publicHostname;
	int publicPort;
	char* sshHostname;
	int sshPort;
	char* sslHostname;
	int timeOut;
	int sslPort;
	int bufferSize;
	int niceLevel;
	char* username;						// Username to change into
};

void init_sockaddr (struct sockaddr_in *name,
     	            const char *hostname,
        		    int port);
int writeall(int socket, void* buffer, size_t bytes);
int bridgeConnection(int remoteSocket, int localSocket,
					 unsigned char* readBuffer, struct timeval* timeOut);
void* bridgeThread(void* arg);
int daemonize(const char* name);
void splitHostPort(char* hostPort, char** hostPart, int* portPart);
int parseCommandLine(int argc, char* argv[]);
        		  
#endif /*CSSLH_H_*/
