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


#ifndef READWRITE_H_
#define READWRITE_H_

int writeall(int socket, void* buffer, size_t bytes);
ssize_t redirectData(int fromSocket, int toSocket, void* buffer);

#endif /*READWRITE_H_*/
