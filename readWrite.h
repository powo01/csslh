#ifndef READWRITE_H_
#define READWRITE_H_

int writeall(int socket, void* buffer, size_t bytes);
ssize_t redirectData(int fromSocket, int toSocket, void* buffer);

#endif /*READWRITE_H_*/
