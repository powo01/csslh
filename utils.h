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


#ifndef UTILS_H_
#define UTILS_H_

void resolvAddress(const char* hostname,
		   const char* port,
		   struct addrinfo** res);

int daemonize(const char* name);

int modifyClientThreadCounter(int delta);

#endif /*UTILS_H_*/
