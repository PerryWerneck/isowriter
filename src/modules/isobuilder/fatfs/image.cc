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
  * @brief Implements fatfs image.
  */

 #define LOG_DOMAIN "fatfs"
 #include <config.h>

 #include <udjat/defs.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/intl.h>
 #include <udjat/tools/properties.h>
 #include <fatfs/ff.h>
 #include <fatfs/diskio.h>
 #include <udjat/tools/url.h>
 #include <reinstall/modules/fatfs.h>
 #include <stdexcept>

 #ifdef HAVE_UNISTD_H
	#include <unistd.h>
 #endif

 #include <fcntl.h>
 #include <sys/types.h>

 using namespace Udjat;
 using namespace std;

 namespace FatFS {

	Image::Image(const Settings &settings, int filehandle) : fd{filehandle} {

		if(fd < 0) {
			throw runtime_error(_("Invalid file handle"));
		}

		if(disk_ioctl(0, CTRL_BIND_FD, &fd) != RES_OK) {
			::close(fd);
			throw runtime_error(_("Cant bind fatfs to disk image"));
		}

		{
			// Create a single FAT partition covering the whole image
			BYTE work[FF_MAX_SS];
			memset(work, 0, sizeof(work));
			auto rc = f_fdisk(0, settings.plist, work);
			if(rc != FR_OK) {
				throw runtime_error(Logger::Message{"f_fdisk failed with error '{}', ({})", f_strerror(rc), rc});
			}				
		}

		{

			// Format
			BYTE work[FF_MAX_SS];
			memset(work,0,sizeof(work));
			auto rc = f_mkfs("0:", &settings.parms, work, sizeof work);

			if(rc != FR_OK) {
				throw runtime_error(Logger::Message{"f_mkfs failed with error '{}', ({})", f_strerror(rc), rc});
			}				

		}


	}

	static int open_file(const char *filename, unsigned long long length) {

		int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
		if(fd < 0) {
			throw system_error(errno,system_category(),filename);
		}

		if (ftruncate(fd, length) == -1) {
			int err = errno;
			::close(fd);
			throw system_error(err,system_category(),"Error setting the file size");
		}

		return fd;

	}

	Image::Image(const Settings &settings, const char *filename, unsigned long long length) 
		: Image{settings,open_file(filename,length)} {
	}

	Image::~Image() {
		if(fd > 0) {
			::close(fd);
			fd = -1;
		}
	}

	void push_back(const Udjat::URL &url, const char *path) {

	}

 }

//  #include <udjat/tools/properties.h>
//  #include <udjat/tools/file/temporary.h>
//  #include <udjat/tools/intl.h>
//  #include <memory>
//  #include <fcntl.h>
//  #include <sys/stat.h>
//  #include <reinstall/tools/datasource.h>
//  #include <reinstall/disk/abstract.h>
//  #include <reinstall/image.h>
//  #include <reinstall/tools/writer.h>

//  #ifdef HAVE_UNISTD_H
// 	#include <unistd.h>
//  #endif // HAVE_UNISTD_H

//  #ifdef HAVE_FATFS

//  #include <reinstall/modules/fatfs.h>
//  #include <fatfs/ff.h>
//  #include <fatfs/diskio.h>

//  #error Implement f_setlabel using attribute disk-label. 

//  using namespace Udjat;
//  using namespace std;

//  namespace FatFS {

// 	static const struct {
// 		int code;
// 		const char *message;
// 	} errors[] = {
// 		{ FR_OK,                    N_("Succeeded") },
// 		{ FR_DISK_ERR,              N_("A hard error occurred in the low level disk I/O layer") },
// 		{ FR_INT_ERR,               N_("Assertion failed") },
// 		{ FR_NOT_READY,             N_("The physical drive cannot work") },
// 		{ FR_NO_FILE,               N_("Could not find the file") },
// 		{ FR_NO_PATH,               N_("Could not find the path") },
// 		{ FR_INVALID_NAME,          N_("The path name format is invalid") },
// 		{ FR_DENIED,                N_("Access denied due to prohibited access or directory full") },
// 		{ FR_EXIST,                 N_("Access denied due to prohibited access") },
// 		{ FR_INVALID_OBJECT,        N_("The file/directory object is invalid") },
// 		{ FR_WRITE_PROTECTED,       N_("The physical drive is write protected") },
// 		{ FR_INVALID_DRIVE,         N_("The logical drive number is invalid") },
// 		{ FR_NOT_ENABLED,           N_("The volume has no work area") },
// 		{ FR_NO_FILESYSTEM,         N_("There is no valid FAT volume") },
// 		{ FR_MKFS_ABORTED,          N_("The f_mkfs() aborted due to any problem") },
// 		{ FR_TIMEOUT,               N_("Could not get a grant to access the volume within defined period") },
// 		{ FR_LOCKED,                N_("The operation is rejected according to the file sharing policy") },
// 		{ FR_NOT_ENOUGH_CORE,       N_("LFN working buffer could not be allocated") },
// 		{ FR_TOO_MANY_OPEN_FILES,   N_("Number of open files > FF_FS_LOCK") },
// 		{ FR_INVALID_PARAMETER,     N_("Given parameter is invalid") },
// 	};

// 	static const char * error_msg(int rc) {

// 		for(const auto &error : errors) {
// 			if(error.code == rc) {
// #ifdef GETTEXT_PACKAGE
// 				return dgettext(GETTEXT_PACKAGE,error.message);
// #else
// 				return error.message;
// #endif
// 			}
// 		}

// 		return _("Unexpected error from libfatfs");
// 	}

// 	class Image::Disk : private Udjat::File::Temporary, public Reinstall::Abstract::Disk {
// 	private:
// 		FATFS fs;
// 		bool mounted = false;

// 	public:
// 		Disk(const std::shared_ptr<Settings> settings) : Reinstall::Abstract::Disk{Udjat::File::Handler::fd, settings->imglen} {

// 			if(disk_ioctl(0, CTRL_BIND_FD, &fd) != RES_OK) {
// 				throw runtime_error(_("Cant bind fatfs to disk image"));
// 			}

// 			{

// 				// Format
// 				static const MKFS_PARM parm = {FM_FAT32, 0, 0, 0, 0};

// 				BYTE work[FF_MAX_SS];
// 				memset(work,0,sizeof(work));
// 				auto rc = f_mkfs("0:", &parm, work, sizeof work);

// 				if(rc != FR_OK) {
// 					throw runtime_error(Logger::Message{ _("Unexpected error '{}' on f_mkfs"), error_msg(rc)});
// 				}

// 			}

// 		}

// 		void mount() {
// 			auto rc = f_mount(&fs, "0:", 1);
// 			if(rc != FR_OK) {
// 				throw runtime_error(Logger::Message{ _("Unexpected error '{}' on f_mount"), error_msg(rc)});
// 			}
// 			mounted = true;
// 		}

// 		void unmount() {
// 			auto rc = f_unmount("0:");
// 			if(rc != FR_OK) {
// 				throw runtime_error(Logger::Message{ _("Unexpected error '{}' on f_umount"), error_msg(rc)});
// 			}
// 			mounted = false;
// 		}

// 		inline unsigned long long length() const {
// 			return File::Handler::length();
// 		}

// 		inline size_t block_size() const {
// 			return File::Handler::block_size();
// 		}

// 		inline size_t read(unsigned long long offset, void *contents, size_t length) {
// 			return File::Handler::read(offset,contents,length);
// 		}

// 		virtual ~Disk() {
// 			if(mounted) {
// 				Logger::String{"Forcing unmount of FAT image"}.warning();
// 				auto rc = f_unmount("0:");
// 				if(rc != FR_OK) {
// 					Logger::Message{ _("Unexpected error '{}' on f_umount"), error_msg(rc)}.error();
// 				}
// 			}
// 		}

// 	};

// #ifdef BUILD_LEGACY
// 	Image::Image(Reinstall::Builder *builder, std::shared_ptr<Settings> s)
// 		: Reinstall::Abstract::Image{builder} {

// 		settings = s;
// 		disk = make_shared<Disk>(settings);

// 	}
// #else
// 	Image::Image(Reinstall::Builder *builder, std::shared_ptr<Settings> s)
// 		: Reinstall::Abstract::Image{builder}, settings{s}, disk{make_shared<Disk>(settings)} {
// 	}
// #endif // BUILD_LEGACY

// 	Image::~Image() {

// 	}

// 	void Image::append(std::shared_ptr<Reinstall::DataSource> source) {

// 		const char *to = source->path();

// 		FIL fdst;
// 		{
// 			int rc;

// 			// Create directories
// 			{
// 				const char *last = strrchr(to,'/');
// 				const char *ptr = to;
// 				while(ptr < last) {
// 					const char *next = strchr(ptr+1,'/');
// 					string path{to,(size_t)(next-to)};

// 					rc = f_mkdir(path.c_str());
// 					if(rc != FR_OK && rc != FR_EXIST) {
// 						throw runtime_error(Logger::Message{_("Unable to create path fat://{} ({})"),path.c_str(),error_msg(rc)});
// 					}

// 					ptr = next;
// 				}
// 			}

// 			// Open file
// 			rc = f_open(&fdst, to, FA_WRITE | FA_CREATE_ALWAYS);
// 			if(rc != FR_OK) {
// 				Logger::String{"f_open(",to,") failed with rc=",rc}.error();
// 				throw runtime_error(Logger::Message{_("Unable to open fat://{} ({})"),to,error_msg(rc)});
// 			}

// 		}

// 		try {

// 			auto progress = Udjat::Dialog::Progress::getInstance();
// 			progress->url(String{"fat://",to}.c_str());

// 			source->save([&fdst,progress,to](unsigned long long current, unsigned long long total, const void *buffer, size_t len) -> bool {

// 				unsigned int wrote = 0;
// 				const BYTE *ptr = (const BYTE *) buffer;

// 				progress->set((uint64_t) (current + len), (uint64_t) total);

// 				while(len > 0) {
// 					int rc = f_write(&fdst, ptr, (unsigned int) len, &wrote); 
// 					if(rc != FR_OK) {
// 						throw runtime_error(Logger::Message{_("Error '{}' writing to fat://{}"),error_msg(rc),to});
// 					}
// 					len -= wrote;
// 					ptr += wrote;
// 				}

// 				return true;
// 			});
// 			progress->done();

// 		} catch(...) {
// 			f_close(&fdst);
// 			throw;
// 		}

// 		f_close(&fdst);

// 	}

// 	void Image::append(const char *from, const char *to) {

// 		if(*to == '.') {
// 			to++;
// 		}

// 		FIL fdst;

// 		int rc = f_open(&fdst, to, FA_WRITE | FA_CREATE_ALWAYS); 
// 		if(rc != FR_OK) {
// 			throw runtime_error(Logger::Message{_("Error '{}' opening fat://{}"),error_msg(rc),to});
// 		}

// 		try {

// 			// http://elm-chan.org/fsw/ff/doc/open.html

// 			int fd = ::open(from,O_RDONLY);
// 			if(fd < 0) {
// 				throw system_error(errno, system_category(), from);
// 			}

// 			struct stat st;
// 			if(::fstat(fd,&st) != 0) {
// 				::close(fd);
// 				throw system_error(errno, system_category(), from);
// 			}

// 			if(st.st_blksize > 4096) {
// 				st.st_blksize = 4096;
// 			}

// 			BYTE buffer[st.st_blksize];

// 			unsigned int current = 0;
// 			while(current < st.st_size) {

// 				auto len = ::read(fd,buffer,st.st_blksize);
// 				if(len < 1) {
// 					::close(fd);
// 					throw system_error(errno, system_category(), from);
// 				}

// 				current += len;

// 				unsigned int wrote = 0;
// 				const BYTE *ptr = buffer;

// 				while(len > 0) {
// 					int rc = f_write(&fdst, ptr, (unsigned int) len, &wrote);
// 					if(rc != FR_OK) {
// 						::close(fd);
// 						throw runtime_error(Logger::Message{_("Error '{}' writing to fat://{}"),error_msg(rc),to});
// 					}
// 					len -= wrote;
// 					ptr += wrote;
// 				}

// 			}

// 			::close(fd);

// 		} catch(...) {
// 			f_close(&fdst);
// 			throw;
// 		}

// 		f_close(&fdst);

// 	}

// 	void Image::pre(Udjat::Abstract::Object &) {
// 		Logger::String{"Opening disk image"}.info();
// 		disk->mount();
// 	}

// 	void Image::post(Udjat::Abstract::Object &) {
// 		Logger::String{"Closing disk image"}.info();
// 		disk->unmount();
// 	}

// 	void Image::write() {

// 		Logger::String{"Preparing to write image"}.info();

// 		unsigned long long total = disk->length();
// 		size_t buflen = disk->block_size();

// 		auto &writer = Reinstall::Writer::getInstance();
// 		writer.size(total);

// 		writer.open(Reinstall::Dialog{});

// 		auto progress = Udjat::Dialog::Progress::getInstance();
// 		progress->url(writer.url());

// 		char buffer[buflen];

// 		unsigned long long current = 0LL;
// 		while(current < total) {

// 			progress->set((uint64_t) current, (uint64_t) total);

// 			auto length = (total - current);
// 			if(length > buflen) {
// 				length = buflen;
// 			}

// 			auto bytes = disk->read(current,buffer,length);
// 			if(bytes == 0) {
// 				throw runtime_error("Unexpected EOF reading fat image");
// 			}

// 			writer.write(current, buffer, bytes);

// 			current += bytes;
// 		}

// 		progress->set((uint64_t) total, (uint64_t) total);
// 		progress->title(_( "Finalizing" ));
// 		progress->done();

// 		writer.close();

// 	}

//  }
//  #endif // HAVE_FATFS


