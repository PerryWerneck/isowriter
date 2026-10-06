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
 #include <udjat/tools/url.h>
 #include <reinstall/tools/repository.h>
 #include <reinstall/progress.h>
 #include <udjat/tools/intl.h>
 #include <stdexcept>
 #include <udjat/tools/logger.h>
 #include <private/html_parser.hpp>
 #include <udjat/tools/file/path.h>

 #ifdef HAVE_UNISTD_H
 	#include <unistd.h>
 #endif // HAVE_UNISTD

 #ifdef HAVE_ZLIB
     #include <zlib.h>
 #endif // HAVE_ZLIB

 using namespace Udjat;
 using namespace std;

 namespace Reinstall {

	std::string Repository::hostname;

	/// @brief Build datasource.
	/// @param remote URL for remote files.
	/// @param local URL for local files.
	Repository::Repository(const char *name, const char *remote, const char *local) : std::string{name}, source{remote, local} {

		sanitize(source.remote);

		if(source.local.empty() && source.remote.empty()) {
			throw runtime_error("At least one URL is required");
		}

	}

	/// @brief Build URL using properties.
	/// @param props The properties for URL.
	Repository::Repository(const Udjat::Properties &props) : std::string{props["name"].c_str()}, source{props} {

		allow_cache = props.get("allow-cache",true);
		try_index_gz = props.get("try-index-gz	",true);

		sanitize(URL{props["url"].c_str()});

		if(source.local.empty() && source.remote.empty()) {
			throw runtime_error("At least one URL is required");
		}

	}

	Repository::~Repository() {

	}

	void Repository::sanitize(const Udjat::URL &url) {

		source.sanitize(url);
		if(!(hostname.empty() || source.remote.empty())) {
			// Force hostname to pre-fixed one.
			source.remote.hostname(hostname.c_str());
		}

	}

	void Repository::url(const char *install) {

	}

	void Repository::host(const char *h) {
		hostname = h;
	}

	void Repository::load(const char *path, std::vector<DataSource::Item> &itens) {

		// Implementation for loading items from the repository

		if(files.empty()) {
			reset();
		}

		debug("Loading repository index for path ",path);
		for(const auto &file : files) {

			if(file.has_prefix(path)) {
				debug("Adding file ",file.c_str());
				DataSource::Item item{source.remote.c_str(), source.local.c_str()};
				if(!item.remote.empty()) {
					item.remote += file.c_str();
				}
				if(!item.local.empty()) {
					item.local += file.c_str();
				}
				item.path = file;
				itens.push_back(item);

			}

		}

	}

	void Repository::absolute(Udjat::URL &remote, Udjat::URL &local) {

		// Setup remote
		{
			String path{remote.c_str()};
			if(path.c_str()[0] == '/' || path.c_str()[0] == '.') {
				// It's a relative URL, adjust it to be absolute.
				if(source.remote.empty()) {
					remote.clear();
				} else {
					remote = source.remote.c_str();
					remote += path.c_str();
				}
				debug("Remote URL adjusted to absolute: '",remote.c_str(),"'");
			}
		}

		// Setup local
		{
			String path{local.c_str()};
			if(path.c_str()[0] == '/' || path.c_str()[0] == '.') {
				// It's a relative URL, adjust it to be absolute.
				if(source.local.empty()) {
					local.clear();
				} else {
					local = source.local.c_str();
					local += path.c_str();
				}
				debug("Local URL adjusted to absolute: '",local.c_str(),"'");
			}
		}

	}


	// Udjat::URL Repository::url(bool rm) {

	// 	if(rm || source.local.empty()) {

	// 		// Get remote URL

	// 		if(!strcasecmp(source.remote.scheme().c_str(),"slp")) {
	// 			// Resolve SLP
	// 			throw runtime_error("SLP support is not implemented yet");
	// 		}

	// 		return source.remote;
	// 	}

	// 	return source.local;

	// }


//  #include <config.h>
//  #include <udjat/defs.h>
//  #include <udjat/tools/properties.h>
//  #include <udjat/tools/object.h>
//  #include <udjat/tools/intl.h>
//  #include <udjat/ui/progress.h>
//  #include <udjat/tools/file/path.h>
//  #include <udjat/tools/configuration.h>
//  #include <udjat/tools/url.h>
//  #include <udjat/tools/url/handler.h>
//  #include <udjat/tools/configuration.h>

//  #include <reinstall/tools/datasource.h>
//  #include <reinstall/tools/repository.h>
//  #include <private/slpclient.h>
//  #include <private/html_parser.hpp>
//  #include <list>
//  #include <vector>

//  #ifdef HAVE_ZLIB
// 	#include <zlib.h>
//  #endif // HAVE_ZLIB

//  using namespace Udjat;
//  using namespace std;

//  namespace Reinstall {

// 	Repository::Repository(const Udjat::Properties &node) : FileSource{node,false}, KernelParameter{node}, kparm{node}, slpclient{SLPClient::Factory(node)} {

// 		if(!(url.remote && *url.remote)) {
// 			throw runtime_error(Logger::String{"Repository '",name(),"' has no remote URL defined"});
// 		}

// 		Logger::String{"Using '",url.remote,"' as remote path for repository"}.trace(name());

// 		if(!(url.local && *url.local)) {
			
// 			String path{Config::Value<string>{"repository","cachedir",""}.c_str()};
// 			if(path.empty()) {
// 				throw runtime_error(Logger::String{"Repository '",name(),"' has no cache defined"});
// 			}

// 			FileSource::expand(path,node);
// 			path.expand(node);

// #ifdef DEBUG
// 			debug("Expanding path ",path.c_str());
// 			if(strchr(path.c_str(),'$')) {
// 				throw logic_error("Error expanding variable");
// 			}
// #endif

// 			url.local = path.as_quark();
// 			Logger::String{"Using '",url.local,"' as local path for repository"}.trace(name());

// 		}

// 		if(!(kparm.slp && *kparm.slp) && kparm.enabled) {

// 			const char *srvc = slpclient->service();
// 			if(!(srvc && *srvc)) {

// 				// No SLP for this repository.
// 			 	Logger::String{"SLP service name is not defined, disabling it for this repository"}.warning(name());

// 			} else if(!(kparm.slp && *kparm.slp)) {
				
// 				Logger::Message url{Config::Value<string>{"kernel-parameters","slp","slp://?{}&auto=1"}.c_str(),srvc};
// 				kparm.slp = url.as_quark();

// 			}

// 		}

// 	}

// 	Repository::~Repository() {
// 	}

// 	bool Repository::operator==(const Repository &repo) const noexcept {

// 		if(strcasecmp(name(),repo.name())) {
// 			return false;
// 		}

// 		if(strcasecmp(url.remote,repo.url.remote)) {
// 			return false;
// 		}

// 		if(slpclient.get() && repo.slpclient.get()) {
// 			return *slpclient == *repo.slpclient;
// 		}

// 		return true;
// 	}

	static void parse_index_html(const char *name, const char *root, const URL &url, std::vector<String> &files) {

		Logger::String{"Loading ",url.c_str()}.trace(name);

		auto progress = Progress::Factory();
		progress->set(url.c_str());
		String response = url.get([progress](uint64_t current, uint64_t total){
			progress->set(current,total);
			return false;
		});

		if(response.empty()) {
			throw runtime_error("Empty response from server");
		}

		HtmlParser parser;
		shared_ptr<HtmlDocument> doc = parser.Parse(response.c_str(), response.size());
		if(!doc) {
			throw runtime_error("Error parsing HTML");
		}

		std::vector<shared_ptr<HtmlElement>> elements = doc->GetElementByTagName("a");
		for(auto &element : elements) {

			String href = element->GetAttribute("href");
			if(href.empty() || href[0] == '?' || href[0] == '/' || href.has_prefix("http://") || href.has_prefix("https://")) {	
				continue;
			}	

			if(href[href.size()-1] == '/') {
				parse_index_html(
					name,
					String{root,href.c_str()}.c_str(),
					URL{url.c_str(),href.c_str()},
					files
				);
			} else {

				debug("Adding file ",String{root,href.c_str()}.c_str());
				files.emplace_back(String{root,href.c_str()}.c_str());

			}

		}

	}

	bool Repository::index(const char *filename) {

#ifdef HAVE_ZLIB
		gzFile fd = gzopen(filename, "r");
		if(!fd) {
			throw runtime_error("Error opening INDEX.gz");
		}

		char buffer[4096];
		memset(buffer,0,4096);
		while(gzgets(fd,buffer,4095)) {
			for(size_t ix = 0; ix < 4096 && buffer[ix]; ix++) {
				if(buffer[ix] < ' ') {
					buffer[ix] = 0;
				}
			}

			const char *ptr = buffer;
			if(*ptr == '.') {
				ptr++;
			}
			if(*ptr != '/') {
				throw runtime_error(Logger::String{"Invalid filename in INDEX.gz: '",buffer,"'"});
			}

//			debug("Adding file ",ptr);
			files.emplace_back(ptr);
		}

		gzclose(fd);

		Logger::String{"Got ",files.size()," filenames from repository index."}.trace(c_str());

		return true;
#else
		return false;
#endif // HAVE_ZLIB
	}

	void Repository::reset() {

		debug("Loading repository index from ",source.remote.c_str());
		
		// Clear the repository contents.
		files.clear();

		// TODO: If we have SLP support, try to get the repository URL from SLP.


		// Reload index
		files.clear();

#ifdef HAVE_ZLIB
		if(try_index_gz) {

			debug("Trying index.gz");

			// Try INDEX.gz

			URL url = source.remote;
			url += "INDEX.gz";

			Logger::String{"Searching for ",url.c_str()}.trace(c_str());

			try {

				auto progress = Progress::Factory();
				progress->url(_("Loading repository index"));
				string filename;

				if(!source.local.empty()) {

					// Has local path, update file.
					debug("Using local file");

					filename = source.local.path().c_str();
					File::Path::mkdir(filename.c_str());
					filename += "INDEX.gz";

					url.get(filename.c_str(),[&progress](uint64_t current, uint64_t total){
						progress->set(current,total);
						return false;
					});
					progress->done();
					index(filename.c_str());
					return;

				} else {

					// No local path, use cache.
					debug("Using remote file");
					filename = url.tempfile([&progress](uint64_t current, uint64_t total){
						progress->set(current,total);
						return false;
					});
					progress->done();
					index(filename.c_str());
					unlink(filename.c_str());
					return;
				}

			} catch(const std::exception &e) {

				Logger::String{url.c_str(),": ",e.what()}.error(c_str());

			}


		}
#endif // HAVE_ZLIB

		// TODO: Parse jsontable, example: https://download.opensuse.org/download/tumbleweed/repo/oss/boot/x86_64/?jsontable

		// Parse index.html
		parse_index_html(c_str(),"./",URL{source.remote.c_str(),"/"},files);

	}


// 	const char * Repository::remote() const {

// 		const char *url = slpclient->url();
// 		if(url && *url) {
// 			return url;
// 		}

// 		return FileSource::remote();
// 	}

// 	std::string Repository::value(const Udjat::Abstract::Object &object) const {

// 		String value;

// 		if(kparm.slp && *kparm.slp) {
// 			const char *url = slpclient->url();
// 			if(url && *url) {
// 				value = kparm.slp;
// 			}
// 		}

// 		if(value.empty()) {
// 			value = remote();
// 		}

// 		value.expand(object);

// 		if(value.empty()) {
// 			throw logic_error(Logger::Message{_("Kernel parameter for repository '{}' has an empty value"),name()});
// 		}

// 		return value;
// 	}

// 	void Repository::preset(const char *arg) {
// 		const char *ptr = strchr(arg,'=');

// 		if(!ptr) {
// 			ptr = strchr(arg,':');
// 		}

// 		if(ptr) {

// 			preset(string{arg,(size_t) (ptr-arg)}.c_str(),ptr+1);

// 		} else {

// 			Config::Value<string> target{"install-targets",arg};
// 			if(target.empty()) {
// 				throw std::runtime_error{Logger::Message(_("No target found for '{}', please check your configuration"),arg)};
// 			}

// 			preset("install",target.c_str());

// 		}

// 	}

//  }

 }

