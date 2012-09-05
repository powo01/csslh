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


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "settings.h"
#include "readWrite.h"

int writeall(int socket, void* buffer, size_t bytes)
{
	int rtn = FALSE;

	ssize_t alreadyWriteBytes = 0;

	while(alreadyWriteBytes < bytes)
	{
		ssize_t writeBytes = send(socket,
				buffer + alreadyWriteBytes,
				 bytes - alreadyWriteBytes, 0);

		if(writeBytes > 0)
		{
			alreadyWriteBytes += writeBytes;
			if(alreadyWriteBytes == bytes)
				break;
		}
		else
		{
			if(writeBytes == -1)
			{
				if(errno == EINTR)
					continue;

				syslog(LOG_ERR,"%s:%d: Error during write (errno=%d)",
					 __FILE__, __LINE__, errno);
			}

			break;
		}
	}

	if(alreadyWriteBytes == bytes)
	{
		fsync(socket);
		rtn = TRUE;
	}
	
	return(rtn);
}

ssize_t redirectData(int fromSocket, int toSocket, void* buffer)
{
		ssize_t readBytes = recv(fromSocket, buffer, pGetConfig()->bufferSize, 0);

		if(readBytes > 0)
		{
			if(FALSE == writeall(toSocket, buffer, readBytes))
			{
				syslog(LOG_ERR,
						"Problems during write to localSocket");
				readBytes=-1; // error
			}
		}
		
		return(readBytes);
}
