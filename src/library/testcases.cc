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
 #include <memory>

 #ifdef HAVE_FATFS
	#include <fatfs/ff.h>
	#include <fatfs/diskio.h>
 #endif // HAVE_FATFS

 #include <reinstall/tools/builder.h>
 #include <reinstall/tools/datasource.h>

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

 