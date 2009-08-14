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
	int maxClientThreads;
};

void splitHostPort(char* hostPort, char** hostPart, char** portPart);
int parseCommandLine(int argc, char* argv[]);
 
#endif /*SETTINGS_H_*/
