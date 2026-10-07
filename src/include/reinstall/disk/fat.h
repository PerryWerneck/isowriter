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
  * @brief Declare fat 32 disk image.
  */

 #pragma once
 #include <udjat/defs.h>
 #include <fatfs/ff.h>
 #include <functional>

 #include <reinstall/disk/abstract.h>

 namespace IsoWriter {

	namespace Disk {

		class UDJAT_API Fat32 : Abstract::Disk {
		private:
			int fd;
			FATFS fs;

		public:

			Fat32(int fd, unsigned long long szimage = 0);

			/// @brief Open FAT disk image, create it if szimage != 0.
			Fat32(const char *filename, unsigned long long szimage = 0);
			~Fat32();

			bool for_each(const char *dirname, const std::function<bool(const char *filename)> &task) const;

			/// @brief Replace file on fat disk.
			/// @param from	The path for file in local filesystem.
			/// @param to The path for file in fat disk.
			void replace(const char *from, const char *to);

		};


	}

 }
