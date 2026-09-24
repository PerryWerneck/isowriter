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

 #define LOG_DOMAIN "datasource"

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/tools/url.h>
 #include <reinstall/tools/datasource.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/configuration.h>
 #include <cstdio>
 #include <stdexcept>

 #define LOCAL_TMP "#(temp)#"

 using namespace Udjat;
 using namespace std;

 namespace Reinstall {

	DataSource::DataSource(const char *remote, const char *local) {
		
		allow_cache = Config::Value<bool>{"url-handler","allow-cache",true}.get();

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
	DataSource::DataSource(const Udjat::Properties &props) {

		if(props.contains("allow-cache")) {
			allow_cache = props.get("allow-cache",true);
		} else {
			allow_cache = Config::Value<bool>{"url-handler","allow-cache",true}.get();
		}
		
		// TODO: Get local & remote.

	}

	DataSource::~DataSource() {
		if(!strcmp(local.hostname().c_str(),LOCAL_TMP)) {
			debug("Removing temporary file '",local.c_str(),"'");
			if(remove(local.path().c_str())) {
				Logger::String{"Error cleaning '",local.c_str(),"': ",strerror(errno)}.warning();
			}
		}
	}

	bool DataSource::dir() const noexcept {

		String path;

		if(!remote.empty())  {
			path = remote.path();
		} else {
			path = local.path();
		}

		return path[path.size()-1] == '/';

	}

	Udjat::URL DataSource::url() {

		if(dir()) {
			throw logic_error("Invalid usage for directory based datasource");
		}

		if(remote.empty()) {
			return local;
		}

		if(allow_cache) {

			// TODO: Initialize progress bar.
			
			debug("Downloading ",remote.c_str());
			if(local.empty()) {

				// No local, create temporary path.
				auto filename = remote.tempfile([](uint64_t current, uint64_t total){
					// TODO: Update progress bar.
					return false;
				});

				local = String{"file://" LOCAL_TMP "/",filename.c_str()};

			} else {

				// Has local, check if it's updated.
				remote.get(local.path().c_str(),[](uint64_t current, uint64_t total){
					// TODO: Update progress bar.
					return false;
				});
			
			}

			return local;
		}
		
		return remote;

	}


 }

//  #include <udjat/tools/properties.h>
//  #include <udjat/tools/object.h>
//  #include <udjat/tools/file/path.h>
//  #include <udjat/tools/file/handler.h>
//  #include <udjat/tools/file/temporary.h>
//  #include <udjat/tools/string.h>
//  #include <udjat/tools/url.h>
//  #include <udjat/tools/url/handler.h>
//  #include <udjat/tools/object.h>
//  #include <udjat/tools/configuration.h>
//  #include <udjat/tools/intl.h>
//  #include <udjat/ui/progress.h>

//  #include <reinstall/tools/datasource.h>
//  #include <reinstall/tools/repository.h>
//  #include <reinstall/tools/template.h>
//  #include <sys/stat.h>

//  #include <stdexcept>
//  #include <unistd.h>
//  #include <stdio.h>
//  #include <mntent.h>
//  #include <limits.h>

//  using namespace Udjat;
//  using namespace std;

//  namespace Reinstall {

// 	/*
// 	DataSource::Path::Path(const XML::Node &node) {


// 	}
// 	*/

// 	DataSource::DataSource(const DataSource &src)
// 		: Udjat::NamedObject{src.name()}, allow_cache{src.allow_cache}, repository{src.repository}, update_from_remote{src.update_from_remote} {
// 		this->message = src.message;
// 	}

// 	DataSource::DataSource(const Udjat::Properties &node) : Udjat::NamedObject{node} {

// 		allow_cache = XML::AttributeFactory(node,"allow-cache").as_bool(Config::Value<bool>{"url-handler","allow-cache",true}.get());

// #ifdef DEBUG
// 		update_from_remote = XML::AttributeFactory(node,"update-from-remote").as_bool(false);
// #else
// 		update_from_remote = XML::AttributeFactory(node,"update-from-remote").as_bool(update_from_remote);
// #endif // DEBUG

// 	}

// 	DataSource::~DataSource() {
// 	}

// 	const char * DataSource::path() const {

// 		const char *path = local();

// 		if(path[0] == '.') {
// 			return path+1;
// 		}

// 		if(path[0] == 0) {

// 			path = remote();

// 			if(path[0] == '.' && Config::Value<bool>{"application","legacy",true}) {
// 				Logger::Message{"Local path is empty, using remote '{}' for legacy mode",path}.trace(name());
// 				return path+1;
// 			}

// 			Logger::Message msg{"Unable to handle empty path for {}",remote()};
// 			msg.error(name());
// 			throw logic_error(msg);

// 		}

// 		if(path[0] == '/' && Config::Value<bool>{"application","legacy",true}) {
// 			Logger::Message{"Using relative path for '{}'",path}.warning(name());
// 			return path;
// 		}

// 		Logger::Message msg{"Unable to handle non relative path '{}'",path};
// 		msg.error(name());
// 		throw logic_error(msg);

// 	}

// 	bool DataSource::has_remote() const noexcept {

// 		try {

// 			const char *ptr = remote();

// 			return (ptr && *ptr);

// 		} catch(const std::exception &e) {

// 			Logger::String{e.what()}.error(name());

// 		} catch(...) {

// 			Logger::String{"Unexpected error checking remote path"}.error(name());

// 		}

// 		return false;
		
// 	}

// 	bool DataSource::has_local() const noexcept {

// 		try {

// 			const char *ptr = local();

// 			return (ptr && *ptr);

// 		} catch(const std::exception &e) {

// 			Logger::String{e.what()}.error(name());

// 		} catch(...) {

// 			Logger::String{"Unexpected error checking local path"}.error(name());

// 		}

// 		return false;
// 	}

// 	const Udjat::String DataSource::fspath() const {

// 		if(!has_local()) {
// 			throw logic_error("Unable to get filesystem path without local path");
// 		}

// 		String filename{URL{local()}.path()};

// 		// Sanitize path.
// 		{
// 			size_t pos;
// 			while((pos = filename.find("//")) != string::npos) {
// 				filename.replace(pos,2,"/");
// 			}
// 		}

// 		struct stat st;
// 		if(stat(filename.c_str(),&st)) {
// 			throw system_error(errno, system_category(), Logger::Message({_("Error getting info for '{}'"),filename.c_str()}));
// 		}

// 		FILE *fp;
// 		struct mntent *fs;
// 		fp = setmntent("/etc/mtab", "r");
// 		if (fp == NULL) {
// 			throw system_error(errno, system_category(), _("Error opening /etc/mtab"));
// 		}

// 		struct mntent mnt;
// 		char buf[PATH_MAX*3];
// 		std::string mountpoint;

// 		while ((fs = getmntent_r(fp,&mnt,buf,sizeof(buf))) != NULL) {
// 			debug(fs->mnt_dir);
// 			struct stat stm;
// 			if(stat(fs->mnt_dir,&stm) == 0 && stm.st_dev == st.st_dev) {
// 				mountpoint = fs->mnt_dir;
// 				break;
// 			}

// 		}
// 		endmntent(fp);

// 		if(mountpoint.empty()) {
// 			throw runtime_error(Logger::Message{_("Cant find mountpoint for '{}'"),filename.c_str()});
// 		}

// 		if(mountpoint.size() == 1 && mountpoint[0] == '/') {
// 			Logger::Message{"Mountpoint for '{}' is root, using path as is",filename.c_str()}.trace(name());
// 			return filename;
// 		}

// 		Logger::String{"Got mountpoint '",mountpoint.c_str(),"' for path '",filename.c_str(),"'"}.trace(name());

// 		debug("RESULT= '",filename.c_str()+mountpoint.size(),"'");
		
// 		return Udjat::String{(const char *) (filename.c_str()+mountpoint.size())};

// 	}

// 	std::shared_ptr<Udjat::Dialog::Progress> DataSource::ProgressFactory() const {

// 		auto progress = Udjat::Dialog::Progress::getInstance();

// 		progress->set(url_remote().c_str());		
// 		if(message && *message) {
// 			progress->title(message);
// 		}
// 		return progress;

// 	}
// 	void DataSource::save(const char *path) {

// 		auto progress = ProgressFactory();
// 		auto url = url_remote();

// 		info() << "Downloading " << url.c_str() << endl;

// 		{
// 			string str{path};
// 			auto pos = str.rfind('/');
// 			if(pos == string::npos) {
// 				throw runtime_error("Invalid local path");
// 			}
// 			str.resize(pos);
// 			if(File::Path::mkdir(str.c_str())) {
// 				info() << "New path made: " << str << endl;
// 			}
// 		}

// 		try {

// 			progress->set(url.c_str());

// 			auto handler = url.handler();
// 			handler->update_if_exists(allow_cache);

// 			handler->get(path,[&](uint64_t current, uint64_t total){
// 				progress->set(current,total);
// 				return false;
// 			});

// 			/*
// 			url.get(path,[&](uint64_t current, uint64_t total){
// 				progress->set(current,total);
// 				return false;
// 			});
// 			*/
// 			progress->done();

// 		} catch(const std::exception &e) {

// 			error() << url.c_str() << " -> " << path << ": " << e.what() << endl;
// 			throw;
// 		}

// 	}

// 	std::string DataSource::save(const Udjat::Abstract::Object &object) {

// 		if(has_local()) {

// 			auto url = url_local();
// 			url.expand(object);

// 			std::string filename{url.path().c_str()};

// 			if(!update_from_remote && access(filename.c_str(),R_OK) == 0) {
// 				Logger::String{filename.c_str()," already exists"}.write(Logger::Debug,name());
// 				return filename.c_str();
// 			}

// 			try {

// 				Logger::String{"Downloading ",filename.c_str()}.write(Logger::Debug,name());
// 				DataSource::save(filename.c_str());

// 			} catch(...) {

// 				struct stat sb;
// 				if(stat(filename.c_str(),&sb) != 0 || sb.st_blocks == 0 || (sb.st_mode & S_IFMT) != S_IFREG) {
// 					error() << "Download error, cached file '" << filename << "' not available" << endl;
// 					throw;
// 				}

// 				warning() << "Download error, using cached file '" << filename << "'" << endl;
// 			}

// 			return filename;

// 		} else {

// 			throw logic_error("Standard data source is unable to save without a local file path");

// 		}

// 	}

// 	std::string DataSource::save() {
// 		return save(Udjat::Abstract::Object{});
// 	}

// 	void DataSource::save(const std::function<bool(unsigned long long current, unsigned long long total, const void *buf, size_t length)> &writer) {

// 		Udjat::Abstract::Object object;
// 		const char *local_filename = this->local();

// 		if(local_filename && *local_filename) {

// 			// Has local (cache) file, try to use it.
// 			Udjat::File::Handler{save(object).c_str()}.save(writer);
// 			return;
// 		}

// 		// No cache, download it directly.
// 		auto url = url_remote();

// 		auto handler = url.handler();
// 		handler->update_if_exists(allow_cache);

// 		// url.get(writer);
// 		handler->get(writer);

// 	}

// 	bool DataSource::for_each(const std::function<bool(const char *filename)> &func) const {

// 		if(repository.get() && repository->index()) {
// 			for(const std::string &filename : *repository) {
// 				if(func(filename.c_str())) {
// 					return true;
// 				}
// 			}
// 		}

// 		return false;
// 	}

// 	bool DataSource::dir() const {
// 		const char *ptr = remote();
// 		return (ptr && *ptr && ptr[strlen(ptr)-1] == '/');
// 	}

// 	bool DataSource::for_each(const std::function<bool(std::shared_ptr<DataSource> value)> &func) const {

// 		if(!(repository.get() && repository->index())) {
// 			throw runtime_error(_("Invalid repository"));
// 		}

// 		if(has_local()) {

// 			// Has local path, using standard file source.
// 			std::string required_prefix{local()};

// 			// Setup local path.
// 			if(required_prefix[0] != '.') {
// 				Logger::Message message{"Invalid local path: {}, should start with '.'",required_prefix.c_str()};
// 				if(Config::Value<bool>{"application","legacy",true}) {
// 					const char *ptr = local();
// 					if(ptr[0] == '/') {
// 						required_prefix = ".";
// 						required_prefix += ptr;
// 						message.warning(name());
// 					}
// 				} else {
// 					throw logic_error(message);
// 				}
// 			}

// 			size_t szlocal = required_prefix.size();
// 			for(const auto &path : *repository) {

// 				if(strncmp(required_prefix.c_str(),path.c_str(),szlocal)) {
// 					continue;
// 				}

// 				auto source = make_shared<FileSource>(path.c_str());
// 				source->message = this->message;
// 				source->rename(this->name());
// 				source->update_from_remote = this->update_from_remote;
// 				source->repository = this->repository;

// 				if(func(source)) {
// 					return true;
// 				}

// 			}

// 		} else {

// 			// No local files, using temporary file datasource.

// 			// Get reference file.
// 			const char *prefix = path();
// 			if(prefix[0] != '.') {
// 				prefix = remote();
// 			}
// 			if(prefix[0] != '.') {
// 				throw logic_error("Cant expand non relative repository");
// 			}

// 			size_t szprefix = strlen(prefix);
// 			for(const auto &path : *repository) {

// 				if(strncmp(prefix,path.c_str(),szprefix)) {
// 					continue;
// 				}

// 				auto source = make_shared<TempFileSource>(name(),path,path);
// 				source->message = this->message;
// 				source->repository = this->repository;

// 				if(func(source)) {
// 					return true;
// 				}

// 			}

// 		}

// 		return false;

// 	}

// 	Udjat::URL DataSource::url_local() const {

// 		const char *path = local();

// 		if(path[0] == '.') {
// 			if(!repository) {
// 				throw logic_error("Unable to use relative URLs without repository");
// 			}

// 			URL url{repository->local()};
// 			url += path;

// 			return url;
// 		}

// 		return URL{path};

// 	}

// 	Udjat::URL DataSource::url_remote() const {

// 		const char *path = remote();

// 		if(path[0] == '.' || path[0] == '/') {

// 			if(!repository) {
// 				throw logic_error("Unable to use relative URLs without repository");
// 			}

// 			URL url{repository->remote()};
// 			url += path;

// 			return url;
// 		}

// 		return URL{path};

// 	}

//  }

