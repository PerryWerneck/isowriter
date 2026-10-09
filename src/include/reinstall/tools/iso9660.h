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

 namespace Iso9660 {

	/// @brief Singleton for libisofs.
	// class UDJAT_API Controller {
	// private:
	// 	Controller();
	// 	~Controller();

	// public:
	// 	static Controller & get_instance();

	// };

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

//  #pragma once
//  #include <udjat/defs.h>
//  #include <reinstall/image.h>
//  #include <udjat/tools/properties.h>
//  #include <reinstall/tools/datasource.h>

//  typedef struct Iso_Image IsoImage;
//  typedef struct iso_write_opts IsoWriteOpts;

//  namespace iso9660 {

// 	class UDJAT_API Image : public IsoWriter::Abstract::Image {
// 	public:

// 		/// @brief ISO9660 image definitions.
// 		struct Settings {

// 			const char *system_area = nullptr;
// 			const char *volume_id = nullptr;
// 			const char *publisher_id = nullptr;
// 			const char *data_preparer_id = nullptr;
// 			const char *application_id = nullptr;
// 			const char *system_id = nullptr;
// 			int iso_level = 3;
// 			int rockridge = 1;
// 			int joliet = 1;
// 			int allow_deep_paths = 1;
// 			bool like_iso_hybrid = true;

// 			struct Boot {

// 				const char *catalog = nullptr;

// 				struct ElTorito {

// 					bool enabled = true;
// 					const char *id = nullptr;
// 					const char *image = nullptr;

// 					inline operator bool() const noexcept {
// 						return enabled;
// 					}

// 				} eltorito;

// 			} boot;

// 			Settings(const Udjat::Properties &node);

// 		};

// 		Image(IsoWriter::Builder *builder, std::shared_ptr<Settings> settings);
// 		virtual ~Image();

// 		void pre(Udjat::Abstract::Object &object);

// 		void post(Udjat::Abstract::Object &object);

// 		void write() override;


// 		inline void append(std::list<std::shared_ptr<IsoWriter::DataSource>> &sources) {
// 			IsoWriter::Abstract::Image::append(sources);
// 		}

// 	protected:
// 		// Abstract::Image
// 		void append(const char *from, const char *to) override;

// 	private:
// 		std::shared_ptr<Settings> settings;
// 		IsoImage *image = nullptr;
// 		IsoWriteOpts *opts = nullptr;

// 	};

//  }

