#ifndef UTILS_H_
#define UTILS_H_

void resolvAddress(const char* hostname,
		   const char* port,
		   struct addrinfo** res);

int daemonize(const char* name);

int modifyClientThreadCounter(int delta);

#endif /*UTILS_H_*/
