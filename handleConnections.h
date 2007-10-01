#ifndef HANDLE_CONNECTIONS_H_
#define HANDLE_CONNECTIONS_H_

int handleConnections(int serverSocket);
int bridgeConnection(int remoteSocket, int localSocket,
					 unsigned char* readBuffer, struct timeval* timeOut);
void* bridgeThread(void* arg);
      		  
#endif /*HANDLE_CONNECTIONS_H_*/
