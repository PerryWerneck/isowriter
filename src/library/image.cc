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
  * @brief Implements abstract image.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <reinstall/image.h>
 #include <reinstall/tools/datasource.h>

 namespace Reinstall {

	Image::Image() {
	}
	
	Image::~Image() {
	}

	void Image::load(std::vector<DataSource::Item> &itens) {
		for(auto &item : itens) {
			load(item);
		}
	}

 }

//  #include <config.h>
//  #include <udjat/defs.h>
//  #include <functional>
//  #include <udjat/tools/url.h>
//  #include <reinstall/tools/datasource.h>
//  #include <reinstall/tools/writer.h>
//  #include <reinstall/tools/builder.h>
//  #include <reinstall/tools/template.h>
//  #include <udjat/tools/file.h>
//  #include <udjat/tools/file/temporary.h>
//  #include <reinstall/image.h>
//  #include <udjat/ui/progress.h>
//  #include <udjat/ui/status.h>
//  #include <udjat/tools/intl.h>
//  #include <udjat/tools/configuration.h>
//  #include <reinstall/disk/fat.h>
//  #include <stdexcept>

//  #ifdef HAVE_UNISTD_H
// 	#include <unistd.h>
//  #endif // HAVE_UNISTD_H

//  using namespace Udjat;
//  using namespace std;

//  namespace Reinstall {

// 	const char * Abstract::Image::application_id() noexcept {
// 		return PACKAGE_STRING;
// 	}

//  	Abstract::Image::~Image() {
//  		if(!efibootpart.empty()) {
// 			unlink(efibootpart.c_str());
//  		}
//  	}

//  	static const char *strip_dot(const char *str) {
// 		if(*str == '.' && Config::Value<bool>{"application","legacy",true}) {
// 			return str+1;
// 		}
// 		return str;
//  	}

// 	void Abstract::Image::append(std::shared_ptr<DataSource> source) {

// 		std::string from = source->save();
// 		std::string to = source->path();
// 		auto efi = builder->efi();

// 		if(efi->enabled() && strcmp(strip_dot(to.c_str()),strip_dot(efi->path())) == 0) {
		
// 			if(!efibootpart.empty()) {
// 				throw logic_error("EFI Boot partition already set");
// 			}

// 			// Copy contents to temporary file.
// 			{
// 				debug("Copying EFI boot partition from ",from.c_str()," to temporary file");
// 				efibootpart = Udjat::File::Temporary::create();
// 				Udjat::File::copy(from.c_str(),efibootpart.c_str());
// 				from = efibootpart;
// 			}

// 			// Apply templates in tempfile
// 			{
// 				// Load file names.
// 				std::vector<string> files;
// 				Reinstall::Disk::Fat32 disk{from.c_str()};

// 				// Load filename first to prevent changes.
// 				disk.for_each("",[&files](const char *filename){
// 					debug(filename);
// 					files.push_back(filename);
// 					return false;
// 				});

// 				for(auto file : files) {

// 					auto tmplt = builder->tmplt(file.c_str());
// 					if(tmplt) {
// 						auto from = Udjat::File::Temporary::create();
				
// 						auto progress = Udjat::Dialog::Progress::getInstance();
// 						tmplt->save(*builder,from.c_str(),[progress](uint64_t current, uint64_t total){
// 							return false;
// 						});

// 						Logger::String{"Using template '",tmplt->name(),"' for fat://",file.c_str()}.trace(builder->name());
// 						disk.replace(from.c_str(),file.c_str());

// 					}

// 				}

// 			}

// 		}

// 		append(from.c_str(),to.c_str());

// 	}

// 	void Abstract::Image::write(const std::function<void(unsigned long long offset, const void *contents, unsigned long long length)> &) {
// 		throw runtime_error(_("No write support on selected image"));
// 	}

// 	/*
// 	void Abstract::Image::write() {

// 		Reinstall::Writer &writer = Reinstall::Writer::getInstance();
// 		writer.open(Reinstall::Dialog());

// 		{
// 			auto progress = Udjat::Dialog::Progress::getInstance();
// 			progress->title(_("Writing image"));
// 			progress->set(writer.url());
// 			write([&writer,progress](unsigned long long offset, const void *contents, unsigned long long length){
// 				writer.write(offset,contents,length);
// 			});
// 		}

// 	}
// 	*/

// 	void Abstract::Image::append(list<std::shared_ptr<DataSource>> &sources) {
// 		size_t item = 0;

// 		auto &status = Udjat::Dialog::Status::getInstance();
// 		for(auto &source : sources) {
// 			status.step(++item,sources.size());
// 			append(source);
// 		}
// 		status.step();

// 	}

//  }

