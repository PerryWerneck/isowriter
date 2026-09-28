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
  * @brief Declare FATFS disk image.
  */

 #pragma once
 #include <udjat/defs.h>
 #include <udjat/tools/properties.h>
 #include <reinstall/image.h>
 #include <reinstall/disk/abstract.h>
 #include <memory>

 namespace FatFS {

	class UDJAT_API Image : public Reinstall::Abstract::Image {
	public:

		/// @brief ISO9660 image definitions.
		struct Settings {
			uint8_t type = 0;				///< @brief Image type (FAT/FAT32/EXFAT).
			uint8_t n_fats = 0;				///< @brief Specifies number of FAT copies on the FAT/FAT32 volume.
			uint32_t align = 0;				///< @brief Specifies alignment of the volume data area (file allocation pool, usually erase block boundary of flash memory media) in unit of sector.
			uint32_t n_root = 0;			///< @brief Specifies number of root directory entries on the FAT volume.
			uint32_t au_size = 0;			///< @brief Specifies size of the cluster (allocation unit) in unit of byte.
			uint64_t imglen = 0LL;			///< @brief The image length.
			const char *label = nullptr;    ///< @brief The image label.

			Settings(const Udjat::Properties &node);

			/// @brief Get fat length (in bytes).
			size_t fat_length() const noexcept;

		};

		Image(Reinstall::Builder *builder, const std::shared_ptr<Settings> s);
		virtual ~Image();

		void pre(Udjat::Abstract::Object &object);

		void post(Udjat::Abstract::Object &object);

		void write() override;

		void append(std::shared_ptr<Reinstall::DataSource> source) override;

		inline void append(std::list<std::shared_ptr<Reinstall::DataSource>> &sources) {
			Reinstall::Abstract::Image::append(sources);
		}

	protected:
		void append(const char *from, const char *to) override;

	private:
		std::shared_ptr<Settings> settings;

		/// @brief The FAT disk image on temporary file.
		class Disk;

		std::shared_ptr<Disk> disk;

	};

 }

