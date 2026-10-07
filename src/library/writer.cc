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
  * @brief Implement abstract writer.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <reinstall/tools/writer.h>
 #include <memory>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/intl.h>
 #include <fcntl.h>
 #include <sys/stat.h>
 #include <sys/types.h>

 #ifdef HAVE_UNISTD_H
	#include <unistd.h>
 #endif // HAVE_UNISTD_H

 using namespace Udjat;
 using namespace std;

 namespace Reinstall {

	std::string Writer::devname;

	std::shared_ptr<Writer> Writer::get_instance(unsigned long long length) {

		if(!devname.empty()) {
			return make_shared<Writer>(devname.c_str(), devname, length);
		}

		throw runtime_error("Device detection is unavailable");
	}

	void Writer::set_device_name(const char *path) {
		devname = path;
		Logger::String{"Default output device set to '",devname.c_str(),"'"}.trace();
	}

	Writer::Writer(const char *devname, const std::string &url, unsigned long long length) 
		: Writer{open(devname,O_WRONLY|O_CREAT,0664),url,length} {
	}

	Writer::Writer(int d, const std::string &url, unsigned long long length) : std::string{url}, fd{d} {

		if(fd == -1) {
 			throw system_error(errno, system_category(), Logger::Message({_("Error opening device '{}'"),devname}));
		}

		struct stat sb;
		if (fstat(fd, &sb) == -1) {
 			throw system_error(errno, system_category(), Logger::Message({_("Error getting stat of '{}'"),devname}));
    	}

		if (S_ISREG(sb.st_mode)) {

			debug(devname," is a regular file");

			if(length && fallocate(fd,0,0,length)) {
			 	system_error err{errno, system_category(), Logger::Message({_("Error allocating space for '{}'"),devname})};
				::close(fd);
				fd = -1;
				throw err;
			}

    	} else if(S_ISBLK(sb.st_mode)) {
			debug(devname," is a block device");

		} else {
			::close(fd);
			fd = -1;
 			throw runtime_error(Logger::Message({_("Device '{}' is invalid"),devname}));
		}

	}

	Writer::~Writer() {
		if(fd != -1) {
			::close(fd);
			fd = -1;
		}
	}

	void Writer::write(unsigned long long offset, const char *contents, unsigned long long length) {
		while(length > 0) {
			auto wrote = pwrite(fd,contents,length,offset);
			if(wrote < 0) {
				throw system_error(errno, system_category());
			}
			length -= wrote;
			offset += wrote;
			contents += wrote;
		}
	}


 }


//  #include <config.h>
//  #include <udjat/defs.h>
//  #include <reinstall/tools/writer.h>
//  #include <udjat/tools/logger.h>
//  #include <udjat/tools/intl.h>
//  #include <udjat/ui/progress.h>
//  #include <udjat/tools/configuration.h>
//  #include <reinstall/dialog.h>
//  #include <stdexcept>
//  #include <semaphore.h>

//  #include <fcntl.h>
//  #include <sys/stat.h>

//  #ifndef _WIN32
// 	#include <unistd.h>
//  #endif // _WIN32

//  using namespace Udjat;
//  using namespace std;

//  namespace Reinstall {

// 	std::string Writer::selected;
// 	Writer * Writer::instance = nullptr;

// 	Writer & Writer::getInstance() {
// 		if(instance) {
// 			return *instance;
// 		}
// 		throw runtime_error(_("The device writer is not available"));
// 	}

// 	Writer::Writer(const char *name) : writer_name{name} {
// 		if(instance) {
// 			throw logic_error(_("Writer is already available"));
// 		}
// 		instance = this;
// 	}

// 	Writer::~Writer() {
// 		if(instance == this) {
// 			instance = nullptr;
// 		}
// 	}

// 	void Writer::write(const char *isoname) {

// 		int fd = ::open(isoname,O_RDONLY);
// 		if(fd < 0) {
// 			throw std::system_error(errno,std::system_category(),"Cant open source file");
// 		}

// 		try {

// 			write(fd);

// 		} catch(...) {

// 			::close(fd);
// 			throw;

// 		}

// 		::close(fd);

// 	}

// 	void Writer::write(int fd) {

// 		try {

// 			struct stat st;
// 			if(fstat(fd,&st) != 0) {
// 				throw std::system_error(errno,std::system_category(),"Cant get file stats");
// 			}

// 			size(st.st_size);

// 			{
// 				Reinstall::Dialog dummy;
// 				open(dummy);
// 			}
			
// 			auto progress = Udjat::Dialog::Progress::getInstance();

// 			Logger::String{"Writing image to ",device_url.c_str()}.info();
// 			progress->url(device_url.strip().c_str());

// 			unsigned long long offset = 0;
// 			uint8_t buffer[st.st_blksize];
// 			while(offset < ((unsigned long long) st.st_size)) {

// 				ssize_t bytes = read(fd,buffer,st.st_blksize);
// 				if(bytes < 0) {
// 					throw std::system_error(errno,std::system_category(),"Error reading source file");
// 				} else if(bytes == 0) {
// 					throw runtime_error("Unexpected EOF on source file");
// 				}

// 				// debug(offset,"/",st.st_size);

// 				progress->set((uint64_t) offset,(uint64_t) st.st_size);
// 				Writer::write(offset,buffer,(unsigned long long) bytes);
// 				offset += bytes;

// 			}
// 			progress->set((uint64_t) offset,(uint64_t) st.st_size);
// 			progress->url(_("Finishing..."));
// 			progress->done();


// 		} catch(...) {

// 			close();

// 			throw;
// 		}

// 		close();

// 	}

// 	bool Writer::open(const Reinstall::Dialog &settings) {

// 		if(!selected.empty()) {

// 			// Use pre-selected output.
// 			try {

// 				open(Writer::selected.c_str());

// 			} catch(const std::exception &e) {

// 				Logger::String{e.what()}.error("writer");
// 				close();
// 				throw;

// 			}

// 			this->device_url = selected.c_str();
// 			Logger::String{"Writing image to pre-selected output ",this->device_url.c_str()}.info();

// 			return true;

// 		}

// 		return false;
// 	}

//  }
