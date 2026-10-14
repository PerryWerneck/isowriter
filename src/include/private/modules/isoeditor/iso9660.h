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

 #pragma once

 #include <udjat/defs.h>
 #include <cstdint>

 #define LIBISOFS_WITHOUT_LIBBURN
 #include <libisofs/libisofs.h> 
 
 #include <udjat/tools/properties.h>
 #include <reinstall/tools/datasource.h>
 #include <reinstall/image.h>
 #include <vector>

 namespace Module::IsoEditor {

	class UDJAT_API Image {
	private:
		IsoWriter::DataSource::Item source;
		IsoImage *image = NULL;
		IsoDataSource *dsrc = NULL;
		IsoReadOpts *ropts = NULL;
		IsoWriteOpts *wopts = NULL;
		char system_area[32768];

		/// @brief Sources passed to push_back(). libisofs reads their files
		/// when the image is written, and Item deletes temporary files from
		/// its destructor.
		std::vector<std::shared_ptr<IsoWriter::DataSource::Item>> mapped;

	public:

		/// @brief Open image, download if necessary.
		/// @param source The datasource for image. 
		Image(const char *isoname);
		~Image();

		/// @brief Insert a disk file into the image, like xorriso -map.
		/// @param source Disk object to insert. source->path is the ISO path.
		void map(std::shared_ptr<IsoWriter::DataSource::Item> source);

		/// @brief Write iso image to file.
		void write();

	};

 }
