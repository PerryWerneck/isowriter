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

 #include <config.h>

 #if defined(DEBUG) 

 #include <udjat/defs.h>
 #include <udjat/tools/testsuite.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/string.h>
 #include <memory>
 #include <fcntl.h>
 #include <sys/types.h>

 #ifdef HAVE_UNISTD_H
	#include <unistd.h>
 #endif // HAVE_UNISTD_H

 #ifdef HAVE_FATFS
	#include <fatfs/ff.h>
	#include <fatfs/diskio.h>
 #endif // HAVE_FATFS

 #include <reinstall/tools/builder.h>
 #include <reinstall/tools/datasource.h>
 #include <reinstall/tools/repository.h>

 using namespace Udjat;
 using namespace Reinstall;
 using namespace std;

 UDJAT_API void udjat_register_tests(Udjat::TestSuite &suite) noexcept {

	suite.add(
		PACKAGE_NAME " Tests",
#if defined(HAVE_FATFS)
		 TestSuite::Case{
			"fatfs", "Test FatFS",
			[](std::ostream &stream) {

				auto install = make_shared<Repository>("https://download.opensuse.org/tumbleweed/repo/oss/");
				Builder builder{"test"};

				builder.push_back(make_shared<DataSource>(
					install,
					"/boot/"
				));

				builder.push_back(make_shared<DataSource>(
					install,
					"/EFI/"
				));

				builder.push_back(make_shared<DataSource>(
					install,
					"/x86_64/"
				));

				// Create file
				#define IMAGE_SIZE (1ULL * 1024 * 1024 * 1024) // 1 GB in bytes

				unlink("test_image.fat");
				int fd = open("test_image.fat", O_RDWR | O_CREAT | O_TRUNC, 0644);
				if(fd < 0) {
					throw system_error(errno,system_category(),"test_tmage.fat");
				}

				if (ftruncate(fd, IMAGE_SIZE) == -1) {
					int err = errno;
					::close(fd);
					throw system_error(err,system_category(),"Error setting the file size");
				}

				stream << "Successfully allocated " << IMAGE_SIZE << " (1 GB)." << endl;

				if(disk_ioctl(0, CTRL_BIND_FD, &fd) != RES_OK) {
					throw runtime_error("Cant bind fatfs to disk image");
				}

				{
					// Create a single FAT partition covering the whole image
					LBA_t plist[] = {100, 0, 0, 0};	// 100% on partition 1
					BYTE work[FF_MAX_SS];
					memset(work, 0, sizeof(work));
					auto rc = f_fdisk(0, plist, work);
					if(rc != FR_OK) {
						throw runtime_error(Logger::Message{"f_fdisk failed with error '{}', ({})", f_strerror(rc), rc});
					}				
				}

				{

					// Format
					static const MKFS_PARM parm = {
						FM_FAT32,		// Format option (FM_FAT, FM_FAT32, FM_EXFAT and FM_SFD)
						0, 				// Number of FATs
						0, 				// Data area alignment (sector)
						0, 				// Number of root directory entries
						0				// Cluster size (byte)
					};

					BYTE work[FF_MAX_SS];
					memset(work,0,sizeof(work));
					auto rc = f_mkfs("0:", &parm, work, sizeof work);

					if(rc != FR_OK) {
						throw runtime_error(Logger::Message{"f_mkfs failed with error '{}', ({})", f_strerror(rc), rc});
					}				

				}

				::close(fd);
				return "FatFS test passed";
			}
		},
#endif // HAVE_FATFS 
		 TestSuite::Case{
			"isobuilder", "Test ISOBuilder",
			[](std::ostream &stream) {

				Builder builder{"test"};
				builder.push_back(make_shared<Builder::Kernel>("https://download.opensuse.org/tumbleweed/repo/oss/boot/x86_64/loader/linux"));
				builder.push_back(make_shared<Builder::InitRD>("https://download.opensuse.org/tumbleweed/repo/oss/boot/x86_64/loader/initrd"));


				return "ISOBuilder test passed";
			}
		}
	);

 }

 #endif // DEBUG

 