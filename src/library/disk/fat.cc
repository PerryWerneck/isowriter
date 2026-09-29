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

 static const struct {
	int code;
	const char *message;
 } errors[] = {

	{ FR_OK, 					N_("Succeeded") },
	{ FR_DISK_ERR, 				N_("A hard error occurred in the low level disk I/O layer") },
	{ FR_INT_ERR, 				N_("Assertion failed") },
	{ FR_NOT_READY, 			N_("The physical drive cannot work") },
	{ FR_NO_FILE, 				N_("Could not find the file") },
	{ FR_NO_PATH, 				N_("Could not find the path") },
	{ FR_INVALID_NAME,			N_("The path name format is invalid") },
	{ FR_DENIED, 				N_("Access denied due to prohibited access or directory full") },
	{ FR_EXIST, 				N_("Access denied due to prohibited access") },
	{ FR_INVALID_OBJECT, 		N_("The file/directory object is invalid") },
	{ FR_WRITE_PROTECTED,		N_("The physical drive is write protected") },
	{ FR_INVALID_DRIVE,			N_("The logical drive number is invalid") },
	{ FR_NOT_ENABLED,			N_("The volume has no work area") },
	{ FR_NO_FILESYSTEM,			N_("There is no valid FAT volume") },
	{ FR_MKFS_ABORTED,			N_("The f_mkfs() aborted due to any problem") },
	{ FR_TIMEOUT,				N_("Could not get a grant to access the volume within defined period") },
	{ FR_LOCKED,				N_("The operation is rejected according to the file sharing policy") },
	{ FR_NOT_ENOUGH_CORE,		N_("LFN working buffer could not be allocated") },
	{ FR_TOO_MANY_OPEN_FILES, 	N_("Number of open files > FF_FS_LOCK") },
	{ FR_INVALID_PARAMETER, 	N_("Given parameter is invalid") },
 	{ FR_DISK_ERR, 				N_("A hard error occurred in the low level disk I/O layer") },
	{ FR_INT_ERR, 				N_("Assertion failed") },
	{ FR_NOT_READY, 			N_("The physical drive cannot work") },
	{ FR_NO_FILE, 				N_("Could not find the file") },
	{ FR_NO_PATH, 				N_("Could not find the path") },
	{ FR_INVALID_NAME,			N_("The path name format is invalid") },
	{ FR_DENIED, 				N_("Access denied due to prohibited access or directory full") },
	{ FR_EXIST, 				N_("Access denied due to prohibited access") },
	{ FR_INVALID_OBJECT, 		N_("The file/directory object is invalid") },
	{ FR_WRITE_PROTECTED,		N_("The physical drive is write protected") },
	{ FR_INVALID_DRIVE,			N_("The logical drive number is invalid") },
	{ FR_NOT_ENABLED,			N_("The volume has no work area") },
	{ FR_NO_FILESYSTEM,			N_("There is no valid FAT volume") },
	{ FR_MKFS_ABORTED,			N_("The f_mkfs() aborted due to any problem") },
	{ FR_TIMEOUT,				N_("Could not get a grant to access the volume within defined period") },
	{ FR_LOCKED,				N_("The operation is rejected according to the file sharing policy") },
	{ FR_NOT_ENOUGH_CORE,		N_("LFN working buffer could not be allocated") },
	{ FR_TOO_MANY_OPEN_FILES, 	N_("Number of open files > FF_FS_LOCK") },
	{ FR_INVALID_PARAMETER, 	N_("Given parameter is invalid") },
 };


 namespace Reinstall {

	static const char * error_msg(int rc) {

		for(const auto &error : errors) {
			if(error.code == rc) {
				return error.message;
			}
		}

		return "Unexpected";
	}

	Disk::Fat32::Fat32(int f, unsigned long long szimage) : Abstract::Disk{f,szimage}, fd{f} {

		debug("Opening fatfs on fd=",f);

		struct stat st;
		if(::fstat(fd,&st) != 0) {
			throw system_error(errno, system_category(), "fstat");
		}

		if(!st.st_size) {
			throw runtime_error(_("Empty disk image"));
		}

		if(disk_ioctl(0, CTRL_BIND_FD, &fd) != RES_OK) {
			throw runtime_error(_("Cant bind fatfs to disk image"));
		}

		if(szimage) {

			// Format
			static const MKFS_PARM parm = {FM_FAT32, 0, 0, 0, 0};

			BYTE work[FF_MAX_SS];
			memset(work,0,sizeof(work));
			auto rc = f_mkfs("0:", &parm, work, sizeof work);

			if(rc != FR_OK) {
				throw runtime_error(Logger::Message{ _("f_mkfs has failed: {}"), error_msg(rc)});
			}

		}

		auto rc = f_mount(&fs, "0:", 1);
		if(rc != FR_OK) {
			throw runtime_error(Logger::Message{ _("f_mount has failed: {}"), error_msg(rc)});
		}

	}

	Disk::Fat32::Fat32(const char *filename, unsigned long long szimage)
		: Fat32{open(filename,O_CREAT|O_RDWR,0644),szimage} {
		debug("'",filename,"' opened as fat disk");
	}

	Disk::Fat32::~Fat32() {
		auto rc = f_mount(NULL, "", 0);
		if(rc != FR_OK) {
			Logger::Message{ _("f_mount has failed: "), error_msg(rc)}.error("fatfs");
		}
		::close(fd);
	}

	bool Disk::Fat32::for_each(const char *dirname, const std::function<bool(const char *filename)> &task) const {

		DIR dp;
		if(f_opendir(&dp, dirname) != FR_OK) {
			throw runtime_error(Logger::Message{_("Unable to open fat://{}"),dirname});
		}

		try {

			FILINFO fno;
			f_readdir(&dp, &fno);

			while(fno.fname[0] != 0) {
				string name{dirname};
				name += '/';
				name += fno.fname;

				bool rc = false;
				if(fno.fattrib & AM_DIR) {
					rc = for_each(name.c_str(),task);
				} else {
					rc = task(name.c_str());
				}

				if(rc) {
					f_closedir(&dp);
					return true;
				}

				f_readdir(&dp, &fno);

			}

		} catch(...) {

			f_closedir(&dp);
			throw;
		}

		f_closedir(&dp);

		return false;

	}

	void Disk::Fat32::replace(const char *from, const char *to) {

		FIL fdst;
		if(f_open(&fdst, to, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
			throw runtime_error(Logger::Message{_("Unable to create fat://{}"),to});
		}

		try {

			// http://elm-chan.org/fsw/ff/doc/open.html

			int fd = ::open(from,O_RDONLY);
			if(fd < 0) {
				throw system_error(errno, system_category(), from);
			}

			struct stat st;
			if(::fstat(fd,&st) != 0) {
				::close(fd);
				throw system_error(errno, system_category(), from);
			}

			if(st.st_blksize > 4096) {
				st.st_blksize = 4096;
			}

			BYTE buffer[st.st_blksize];

			unsigned int current = 0;
			while(current < st.st_size) {

				auto len = ::read(fd,buffer,st.st_blksize);
				if(len < 1) {
					::close(fd);
					throw system_error(errno, system_category(), from);
				}

				current += len;

				unsigned int wrote = 0;
				const BYTE *ptr = buffer;

				while(len > 0) {
					if(f_write(&fdst, ptr, (unsigned int) len, &wrote) != FR_OK) {
						::close(fd);
						throw runtime_error(Logger::Message{_("Unable to write fat://{}"),to});
					}
					len -= wrote;
					ptr += wrote;
				}

			}

			::close(fd);

		} catch(...) {
			f_close(&fdst);
			throw;
		}

		f_close(&fdst);

	}

 }
