#ifndef SETTINGS_H_
#define SETTINGS_H_

#define TRUE (1==1)
#define FALSE (2==1)

#define SSH_PORT 22
#define SSL_PORT 443
#define NEW_ROOT_DIR "/tmp"

#define BUFFERSIZE 2048

struct configuration
{
	char* publicHostname;
	char* publicPort;
	char* sshHostname;
	char* sshPort;
	char* sslHostname;
	int timeOut;
	char* sslPort;
	int bufferSize;
	int niceLevel;
	char* username;						// Username to change into
};

void splitHostPort(char* hostPort, char** hostPart, char** portPart);
int parseCommandLine(int argc, char* argv[]);
 
#endif /*SETTINGS_H_*/
