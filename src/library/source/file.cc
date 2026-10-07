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
  * @brief Implements Source repository.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/tools/properties.h>
 #include <udjat/tools/object.h>
 #include <udjat/tools/intl.h>
 #include <udjat/tools/file.h>

 #include <unistd.h>

 #include <reinstall/tools/datasource.h>
 #include <reinstall/tools/repository.h>

 using namespace Udjat;
 using namespace std;

 namespace IsoWriter {

	FileSource::FileSource(const char *path) {
		url.remote = url.local = path;
	}

	void FileSource::expand(Udjat::String &path, const Udjat::Properties &node) {
		path.expand(node);
		path.expand([this,node](const char *key, string &value) -> bool {

			if(!strcasecmp(key,"name")) {
				value = name();
				return true;
			}

			if(!strcasecmp(key,"group-name")) {
				for(auto xml = node; xml; xml = xml.parent()) {
					debug("---> '",xml.name(),"'");
					if(!strcasecmp(xml.name(),"group")) {
						value = xml.attribute("name").as_string();
						return true;
					}
				}
			}

			return false;
		});
	}

	FileSource::FileSource(const Udjat::Properties &node, bool required) : DataSource{node} {

		URL paths[]{
			{node,"local",false},
			{node,"remote",false},
			{node,"url",false}
		};

		// Expand paths.
		for(auto &path : paths) {
			this->expand(path, node);
			path.expand();
		}

		// Build URL for remote
		if(!paths[1].empty()) {
			url.remote = paths[1].as_quark();
		} else if(paths[2].remote()) {
			url.remote = paths[2].as_quark();
		}

		// Build URL for local.
		if(!paths[0].empty()) {
			url.local = paths[0].as_quark();
		} else if(paths[2].local()) {
			url.local = paths[2].as_quark();
		}

		if(url.remote[0] == '.' || url.local[0] == '.' || url.remote[0] == '/' || url.local[0] == '/') {

			// If the URL starts with '/' or '.' then it is relative to the repository.
			repository = Repository::Factory(node);
			if(!url.remote[0]) {
				url.remote = url.local;
			} else if(!url.local[0]) {
				url.local = url.remote;
			}
		}

	}

	FileSource::~FileSource() {
	}

	bool FileSource::has_remote() const noexcept {

		try {

			if(!(url.remote && *url.remote)) {
				return false;
			}

			if(url.remote[0] != '.') {
				return true;
			}

			// Check if repository has local.
			if(repository && repository->has_remote()) {
				return true;
			}

		} catch(const std::exception &e) {

			Logger::String{e.what()}.error(name());

		} catch(...) {

			Logger::String{"Unexpected error"}.error(name());

		}
		return false;
	}


	bool FileSource::has_local() const noexcept {

		try {

			if(!(url.local && *url.local)) {
				return false;
			}

			if(url.local[0] != '.') {
				return true;
			}

			// Check if repository has local.
			if(repository && repository->has_local()) {
				return true;
			}

			if(url.local[strlen(url.local)-1] != '/') {
				Logger::String{"Unable to determine local path for ",url.remote}.trace(name());
			}

		} catch(const std::exception &e) {

			Logger::String{e.what()}.error(name());

		} catch(...) {

			Logger::String{"Unexpected error"}.error(name());

		}
		return false;
	}


	Udjat::Properties find(Udjat::Properties node, const char *nodename, bool required) {

		while(node) {
			if(!strcasecmp(node.name(),nodename)) {
				return node;
			}
			auto child = node.child(nodename);
			if(child) {
				return child;
			}
			node = node.parent();
		}

		if(required) {
			throw runtime_error(Logger::String{"Required node '",nodename,"' is missing"});
		}

		return Udjat::Properties{};

	}

	FileSource::FileSource(const Udjat::Properties &node, const char *nodename, bool required)
		: FileSource{find(node,nodename,required)} {
	}

	const char * FileSource::local() const {
		return url.local;
	}

	const char * FileSource::remote() const {
		// debug("url.remote = '",url.remote,"'");
		return url.remote;
	}

	const char * FileSource::path() const {
		if(url.path && *url.path) {
			return url.path;
		}
		return DataSource::path();
	}


 }


