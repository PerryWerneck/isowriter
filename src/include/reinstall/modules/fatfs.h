/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2026 Perry Werneck <perry.werneck@gmail.com>
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
  * @brief Declare FATFS disk image.
  */

 #pragma once
 #include <udjat/defs.h>
 #include <udjat/tools/properties.h>
 #include <fatfs/ff.h>
 #include <udjat/tools/url.h>
 #include <reinstall/image.h>
 #include <reinstall/tools/datasource.h>

 namespace FatFS {

	class UDJAT_API Image : public IsoWriter::Image {
	public:
		struct Settings {
			MKFS_PARM parms = { FM_FAT32, 0, 0, 0, 0};	///< Format parameter structure used for f_mkfs()
			LBA_t plist[FF_VOLUMES] = { 100, 0, 0, 0 };
		};

		/// @brief Build a new fat disk image.
		/// @param settings Settings for new image.
		/// @param filename The image file name.
		/// @param length The image length.
		Image(const Settings &settings, const char *filename, unsigned long long length);

		~Image();

		/// @brief Add file on fat device or image.
		/// @param url The URL for the source file.
		/// @param path The path for file inside fat image.
		void push_back(const Udjat::URL &url, const char *path);

		inline void load(std::vector<IsoWriter::DataSource::Item> &itens) {
			IsoWriter::Image::load(itens);
		}


	protected:
		void load(IsoWriter::DataSource::Item &item) override;

	private:

		/// @brief The handle of fat device.
		int fd = -1;

		/// @brief Is the device mounted?
		bool mounted = false;

		FATFS fs;

		/// @brief Initialize an empty fat file or device.
		/// @param settings The settings for new image.
		/// @param fd File handle of fat device (or image file).
		Image(const Settings &settings, int fd);

	};

 }

//  #include <reinstall/image.h>
//  #include <reinstall/disk/abstract.h>
//  #include <memory>

//  namespace FatFS {

// 	class UDJAT_API Image : public IsoWriter::Abstract::Image {
// 	public:

// 		/// @brief ISO9660 image definitions.
// 		struct Settings {
// 			uint8_t type = 0;				///< @brief Image type (FAT/FAT32/EXFAT).
// 			uint8_t n_fats = 0;				///< @brief Specifies number of FAT copies on the FAT/FAT32 volume.
// 			uint32_t align = 0;				///< @brief Specifies alignment of the volume data area (file allocation pool, usually erase block boundary of flash memory media) in unit of sector.
// 			uint32_t n_root = 0;			///< @brief Specifies number of root directory entries on the FAT volume.
// 			uint32_t au_size = 0;			///< @brief Specifies size of the cluster (allocation unit) in unit of byte.
// 			uint64_t imglen = 0LL;			///< @brief The image length.
// 			const char *label = nullptr;    ///< @brief The image label.

// 			Settings(const Udjat::Properties &node);

// 			/// @brief Get fat length (in bytes).
// 			size_t fat_length() const noexcept;

// 		};

// 		Image(IsoWriter::Builder *builder, const std::shared_ptr<Settings> s);
// 		virtual ~Image();

// 		void pre(Udjat::Abstract::Object &object);

// 		void post(Udjat::Abstract::Object &object);

// 		void write() override;

// 		void append(std::shared_ptr<IsoWriter::DataSource> source) override;

// 		inline void append(std::list<std::shared_ptr<IsoWriter::DataSource>> &sources) {
// 			IsoWriter::Abstract::Image::append(sources);
// 		}

// 	protected:
// 		void append(const char *from, const char *to) override;

// 	private:
// 		std::shared_ptr<Settings> settings;

// 		/// @brief The FAT disk image on temporary file.
// 		class Disk;

// 		std::shared_ptr<Disk> disk;

// 	};

//  }

