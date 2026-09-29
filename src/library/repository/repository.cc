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
 #include <stdexcept>

 using namespace Udjat;
 using namespace std;

 namespace Reinstall {

	/// @brief Build datasource.
	/// @param remote URL for remote files.
	/// @param local URL for local files.
	Repository::Repository(const char *remote, const char *local) {

		if(remote && *remote) {
			URL u{remote};
			if(u.local()) {
				this->local = u;
			} else {
				this->remote = u;
			}
		}

		if(local && *local) {
			this->local = local;
		}

		if(this->local.empty() && this->remote.empty()) {
			throw runtime_error("At least one URL is required");
		}

	}

	/// @brief Build URL using properties.
	/// @param props The properties for URL.
	Repository::Repository(const Udjat::Properties &props) {


	}

	Repository::~Repository() {

	}

	Udjat::URL Repository::url(bool rm) {

		if(rm || local.empty()) {

			// Get remote URL

			if(!strcasecmp(remote.scheme().c_str(),"slp")) {
				// Resolve SLP
				throw runtime_error("SLP support is not implemented yet");
			}

			return remote;
		}

		return local;

	}

 }

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

// 	static void parse_index_html(const char *name, const char *root, const URL &url, std::vector<std::string> &files) {

// 		Logger::String{"Loading ",url.c_str()}.trace(name);
// 		Dialog::Progress::getInstance()->set(url.c_str());

// 		String response = url.get();

// 		if(response.empty()) {
// 			throw runtime_error("Empty response from server");
// 		}

// 		HtmlParser parser;
// 		shared_ptr<HtmlDocument> doc = parser.Parse(response.c_str(), response.size());
// 		if(!doc) {
// 			throw runtime_error("Error parsing HTML");
// 		}

// 		std::vector<shared_ptr<HtmlElement>> elements = doc->GetElementByTagName("a");
// 		for(auto &element : elements) {

// 			String href = element->GetAttribute("href");
// 			if(href.empty() || href[0] == '?' || href[0] == '/' || href.has_prefix("http://") || href.has_prefix("https://")) {	
// 				continue;
// 			}	

// 			if(href[href.size()-1] == '/') {
// 				parse_index_html(
// 					name,
// 					String{root,href.c_str()}.c_str(),
// 					URL{url.c_str(),href.c_str()},
// 					files
// 				);
// 			} else {

// 				debug("Adding file ",String{root,href.c_str()}.c_str());
// 				files.emplace_back(String{root,href.c_str()}.c_str());

// 			}

// 		}

// 	}

// 	bool Repository::index(const char *filename) {
// #ifdef HAVE_ZLIB
// 		gzFile fd = gzopen(filename, "r");
// 		if(!fd) {
// 			throw runtime_error("Error opening INDEX.gz");
// 		}

// 		char buffer[4096];
// 		memset(buffer,0,4096);
// 		while(gzgets(fd,buffer,4095)) {
// 			for(size_t ix = 0; ix < 4096 && buffer[ix]; ix++) {
// 				if(buffer[ix] < ' ') {
// 					buffer[ix] = 0;
// 				}
// 			}
// //			debug("Adding file ",buffer);
// 			files.emplace_back(buffer);
// 		}

// 		gzclose(fd);

// 		Logger::String{"Got ",files.size()," filenames from repository index."}.trace(name());

// 		return true;
// #else
// 		return false;
// #endif // HAVE_ZLIB
// 	}

// 	bool Repository::index() {

// 		if(!files.empty()) {
// 			return true;
// 		}

// #ifdef HAVE_ZLIB
// 		{
// 			debug("Trying index.gz");

// 			// Try INDEX.gz

// 			URL url = url_remote();
// 			url += "INDEX.gz";

// 			Logger::String{"Searching for ",url.c_str()}.trace(name());

// 			try {

// 				string filename;

// 				if(has_local()) {

// 					// Has local path, update file.
// 					debug("Using local file");

// 					filename = url_local().path();
// 					File::Path::mkdir(filename.c_str());
// 					filename += "INDEX.gz";

// 					auto progress = Dialog::Progress::getInstance();
// 					progress->url(_("Loading repository index"));
// 					url.get(filename.c_str(),[&progress](uint64_t current, uint64_t total){
// 						progress->set(current,total);
// 						return false;
// 					});
// 					progress->done();
// 					return index(filename.c_str());

// 				} else {

// 					// No local path, use cache.
// 					debug("Using remote file");

// 					auto progress = Dialog::Progress::getInstance();
// 					progress->url(_("Loading repository index"));
// 					filename = url.tempfile([&progress](uint64_t current, uint64_t total){
// 						progress->set(current,total);
// 						return false;
// 					});
// 					progress->done();
// 					bool rc = index(filename.c_str());
// 					unlink(filename.c_str());
// 					return rc;
// 				}

// 			} catch(const std::exception &e) {

// 				Logger::String{url.c_str(),": ",e.what()}.error(name());

// 			}


// 		}
// #endif // HAVE_ZLIB

// 		// Parse index.html
// 		parse_index_html(name(),"./",URL{url_remote().c_str(),"/"},files);

// 		return true;
// 	}


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


