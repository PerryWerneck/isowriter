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
  * @brief Implement fat image settings.
  */

 #include <config.h>
 #include <udjat/tools/properties.h>
 #include <udjat/tools/string.h>
 #include <memory>

 #ifdef HAVE_FATFS

 #include <reinstall/modules/fatfs.h>
 #include <fatfs/ff.h>

 using namespace Udjat;
 using namespace std;

 #error TODO: Implement label on fat image.
 
 namespace FatFS {

	// Reference: http://elm-chan.org/fsw/ff/doc/mkfs.html

	static uint64_t image_length(const Udjat::Properties &node) {

		auto size = node["size"];
		if(size.empty()) {
			size = node["length"];
		}

		if(size.empty()) {
			return 0LL;
		}

		return size.as_ull();
		
		// uint64_t imagesize = 0LL;

		// const char *ptr = size.c_str();
		// while(*ptr && isdigit(*ptr)) {
		// 	imagesize *= 10;
		// 	imagesize += (*ptr - '0');
		// 	ptr++;
		// }

		// while(*ptr && isspace(*ptr)) {
		// 	ptr++;
		// }

		// if(*ptr) {
		// 	static const char *units[] = { "B", "KB", "MB", "GB" };

		// 	bool found = false;
		// 	for(const char *unit : units) {
		// 		if(!strcasecmp(ptr,unit)) {
		// 			found = true;
		// 			break;
		// 		}
		// 		imagesize *= 1024;
		// 	}

		// 	if(!found) {
		// 		throw runtime_error(Logger::String{"Unexpected size unit: '",ptr,"'"});
		// 	}

		// }

		// return imagesize;

	}

	Image::Settings::Settings(const Udjat::Properties &node)
		: type{FM_ANY}, n_fats{(uint8_t)
			XML::AttributeFactory(node,"n_fats").as_uint(1)},
			align{XML::AttributeFactory(node,"align").as_uint(0)},
			n_root{XML::AttributeFactory(node,"n_root").as_uint(0)},
			au_size{XML::AttributeFactory(node,"au_size").as_uint(0)},
			imglen{image_length(node)} {

		if(!imglen) {
			throw runtime_error(Logger::String{"Required attribute 'size' is missing or invalid on node ",node.path()});
		}
	}

	size_t Image::Settings::fat_length() const noexcept {

		// Reference: http://elm-chan.org/fsw/ff/res/mkfs.xlsx
		unsigned int csize = 512; // TODO: Get the real value.

		if(type == FM_ANY) {
			if(au_size == 4096) {
				return 8 * n_fats * csize;
			}
			if(au_size == 0) {
				return 1 * n_fats * csize;
			}
		}

		if(type == FM_FAT) {
			if(au_size == 0) {
				return 1 * n_fats * csize;
			}
		}

		// Default
		return 8 * n_fats * csize;

	}

 }
 #endif // HAVE_FATFS

