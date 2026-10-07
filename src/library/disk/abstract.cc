/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2024 Perry Werneck <perry.werneck@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

 /**
  * @brief Implements fat disk.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/tools/intl.h>
 #include <reinstall/disk/fat.h>
 #include <udjat/tools/logger.h>
 #include <system_error>
 #include <sys/stat.h>

 #include <fcntl.h>

 #include <fatfs/ff.h>
 #include <fatfs/diskio.h>

 #ifdef HAVE_UNISTD_H
	#include <unistd.h>
 #endif // HAVE_UNISTD_H

 using namespace Udjat;
 using namespace std;

 namespace IsoWriter {

	Abstract::Disk::Disk(int fd, unsigned long long szimage) {

		if(fd < 0) {
			debug("fd=",fd);
			throw system_error(errno, system_category(), _("Cant open disk device/file"));
		}

		if(szimage && fallocate(fd,0,0,szimage)) {
			throw system_error(errno,system_category(), _("Cant allocate disk image"));
		}

	}

	Abstract::Disk::~Disk() {
	}


 }
