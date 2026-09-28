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
  * @brief Implement iso 9660 image settings.
  */

 #include <config.h>
 #include <udjat/tools/properties.h>
 #include <memory>
 #include <pwd.h>

 #ifdef HAVE_UNISTD_H
	#include <unistd.h>
 #endif // HAVE_UNISTD_H

 #ifdef HAVE_LIBISOFS

 #include <reinstall/modules/iso9660.h>

 using namespace Udjat;
 using namespace std;

 namespace iso9660 {

	Image::Settings::Settings(const Udjat::Properties &node) {

		// std::string name{XML::AttributeFactory(node,"name").as_string("iso-9660")};

		system_area = XML::QuarkFactory(node,"system-area");

		volume_id = XML::QuarkFactory(node,"volume-id");
		if(!*volume_id) {
			volume_id = XML::QuarkFactory(node,"system-name");
		}

		if(!*volume_id) {
			volume_id = XML::QuarkFactory(node,"disk-label");
		}

		publisher_id = XML::QuarkFactory(node,"publisher-id");

		// Use the username as data preparer id if not set.
		struct passwd *pw = getpwuid(getuid());
		if(pw && pw->pw_name && *pw->pw_name) {
			data_preparer_id = String{node,"data-preparer-id",(const char *) pw->pw_name}.as_quark();
		} else {
			data_preparer_id = String{node,"data-preparer-id"}.as_quark();
		}

		application_id = String{node,"application-id",PACKAGE_NAME}.as_quark();
		system_id = String{node,"system-id","LINUX"}.as_quark();

		boot.eltorito.enabled = XML::AttributeFactory(node,"eltorito").as_bool(boot.eltorito.enabled);
		boot.eltorito.id = String{node,"system-name"}.as_quark();

		boot.eltorito.image = String{node,"eltorito-boot-image","/boot/x86_64/loader/isolinux.bin"}.as_quark();
		boot.catalog = String{node,"boot-catalog",(boot.eltorito.enabled ? "/boot/x86_64/loader/boot.cat" : "")}.as_quark();

	}

 }
 #endif // HAVE_LIBISOFS

