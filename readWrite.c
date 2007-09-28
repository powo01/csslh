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
