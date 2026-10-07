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
  * @brief Implements Datasource loader.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/tools/properties.h>
 #include <udjat/tools/object.h>
 #include <udjat/tools/file.h>
 #include <udjat/tools/file/handler.h>
 #include <udjat/tools/file/temporary.h>
 #include <udjat/tools/string.h>
 #include <udjat/tools/url.h>
 #include <udjat/tools/object.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/tools/intl.h>

 #include <reinstall/tools/datasource.h>
 #include <reinstall/tools/repository.h>
 #include <reinstall/tools/template.h>
 #include <sys/stat.h>

 #include <stdexcept>
 #include <unistd.h>

 using namespace Udjat;
 using namespace std;

 namespace IsoWriter {

	void DataSource::load(const Udjat::Properties &node, vector<std::shared_ptr<DataSource>> &sources, const char *nodename) {

		if(nodename) {

			// Has node name, load it.
			for(Udjat::Properties nd = node; nd; nd = nd.parent()) {
				for(Udjat::Properties child = nd.child(nodename); child; child = child.next_sibling(nodename)) {
					sources.push_back(make_shared<FileSource>(child));
				}
			}

		} else {

			// No node name, load <source /> first...
			load(node,sources,"source");

			// ... then load <driver-update-disk />
			class DUD : public IsoWriter::FileSource {
			public:
				DUD(const Udjat::Properties &node, const char *path) : FileSource{node} {
					url.path = path;
				}

			};

			for(Udjat::Properties nd = node; nd; nd = nd.parent()) {
				for(Udjat::Properties child = nd.child("driver-update-disk"); child; child = child.next_sibling("driver-update-disk")) {
					auto path = XML::QuarkFactory(child,"path");
					if(path && *path) {
						auto source = make_shared<DUD>(child,path);
						sources.push_back(source);
					}
				}
			}

		}

	}


 }

