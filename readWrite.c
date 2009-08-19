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

#include "settings.h"
#include "readWrite.h"

int writeall(int socket, void* buffer, size_t bytes)
{
	ssize_t alreadyWriteBytes = 0;
	
	while(bytes > 0 &&
		alreadyWriteBytes < bytes)
	{
		ssize_t writeBytes = write(socket, buffer + alreadyWriteBytes,
								bytes - alreadyWriteBytes);
		
		if(writeBytes > 0)
			alreadyWriteBytes += writeBytes;
		else
			break;
	}
	
	return(alreadyWriteBytes == bytes);
}

ssize_t redirectData(int fromSocket, int toSocket, void* buffer)
{
		extern struct configuration settings;
		ssize_t readBytes = read(fromSocket, buffer, settings.bufferSize);

		if(readBytes > 0)
		{
			if(0 != writeall(toSocket, buffer, readBytes))
			{
				fprintf(stderr,
						"Problems during write to localSocket");
			}
		}
		
		return(readBytes);
}
