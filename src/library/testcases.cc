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
	#include <reinstall/modules/fatfs.h>
 #endif // HAVE_FATFS

 #ifdef HAVE_LIBISOFS
	#include <reinstall/tools/iso9660.h>
 #endif // HAVE_LIBISOFS

 #include <reinstall/tools/builder.h>
 #include <reinstall/tools/datasource.h>
 #include <reinstall/tools/repository.h>
 #include <reinstall/progress.h>
 #include <reinstall/tools/template.h>

 using namespace Udjat;
 using namespace Reinstall;
 using namespace std;

 UDJAT_API void udjat_register_tests(Udjat::TestSuite &suite) noexcept {

	suite.add(
		PACKAGE_NAME " Tests",
		 TestSuite::Case{
			"repository", "Test repository",
			[](std::ostream &stream) {

				auto install = make_shared<Repository>("install","https://download.opensuse.org/tumbleweed/repo/oss/");
				install->reset();

				return "Repository test passed";
			}
		},
#if defined(HAVE_FATFS)
		 TestSuite::Case{
			"fatbuilder", "Test FatBuilder",
			[](std::ostream &stream) {

				auto install = make_shared<Repository>("install","https://download.opensuse.org/tumbleweed/repo/oss/");
				Builder builder{"test"};

				builder.push_back(make_shared<DataSource>(
					"EFI",
					install,
					"/EFI/"
				));

				builder.push_back(make_shared<DataSource>(
					"boot",
					install,
					"/boot/"
				));

				// Create file
				#define IMAGE_SIZE (1ULL * 1024 * 1024 * 1024) // 1 GB in bytes

				unlink("/tmp/test.iso");

				FatFS::Image::Settings settings;
				FatFS::Image disk{settings,"/tmp/test.iso",IMAGE_SIZE};

				std::vector<DataSource::Item> itens;
				builder.load(itens);

				disk.load(itens);

				return "FatFS test passed";
			}
		},
#endif // HAVE_FATFS 
#ifdef HAVE_LIBISOFS
		 TestSuite::Case{
			"isoeditor", "Test ISOEditor",
			[](std::ostream &stream) {

				DataSource source{
					"netinstall",
					"https://download.opensuse.org/tumbleweed/iso/openSUSE-Tumbleweed-NET-x86_64-Current.iso",
					"file:///tmp/iso-editor-source.iso"
				};

				DataSource::Item isofile;
				source.load(isofile);

				stream	<< "From '" << isofile.remote.c_str() << "'" << endl
						<< "To   '" << isofile.local.c_str() << "'" << endl;

				stream << "Loading image" << endl;
				Iso9660::Image image{isofile.save().c_str()};

				// Apply template
				Reinstall::Template grubcfg{"grub2","*/grub.cfg"};

				// Save template to apply contents.
				grubcfg.save();
				stream << "Replacing '" << grubcfg.path << "'" << endl;

				stream << "Writing modified image" << endl;
				image.write("/tmp/test.iso");

				return "ISOEditor test passed";
			}
		},
#endif // HAVE_LIBISOFS
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

 