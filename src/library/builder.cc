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
  * @brief Implements abstract builder.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <reinstall/tools/builder.h>
 #include <reinstall/tools/datasource.h>
 #include <udjat/tools/properties.h>
 #include <memory>

 using namespace Udjat;
 using namespace std;

 namespace Reinstall {

	Builder::Builder(const Udjat::Properties &props) : Reinstall::Action{props["name"].as_quark()} {
	}

	bool Builder::push_back(std::shared_ptr<Reinstall::DataSource> source) {
		
		{
			auto k = dynamic_pointer_cast<Kernel>(source);
			if(k) {

				// It's a kernel object, only insert the first one.
				if(kernel) {
					Logger::String{"Kernel was already inserted"}.write(Logger::Debug);
					debug("Kernel was already inserted");
					return false;
				}
				kernel = k;
			}
		}

		{
			auto i = dynamic_pointer_cast<InitRD>(source);
			if(i) {
				// It's an InitRD object, only insert the first one.
				if(initrd) {
					Logger::String{"InitRD was already inserted"}.write(Logger::Debug);
					return false;
				}
				initrd = i;
			}
		}

		sources.push_back(source);
		return true;
	}

	void Builder::load(std::vector<DataSource::Item> &itens) {
		for(auto source : sources) {
			source->load(itens);
		}
	}

	// bool Builder::for_each(const std::function<bool(const Udjat::URL &from, const char *to)> &task) const {
	// 	for(auto source : sources) {
	// 		if(source->for_each(task)) {
	// 			return true;
	// 		}
	// 	}
	// 	return false;
	// }

 }

//  #include <udjat/tools/properties.h>
//  #include <reinstall/tools/builder.h>
//  #include <udjat/tools/intl.h>
//  #include <reinstall/tools/datasource.h>
//  #include <reinstall/tools/template.h>
//  #include <udjat/tools/file/temporary.h>
//  #include <udjat/ui/status.h>
//  #include <udjat/ui/progress.h>
//  #include <udjat/tools/logger.h>
//  #include <udjat/tools/configuration.h>
//  #include <vector>
//  #include <unistd.h>

//  using namespace Udjat;
//  using namespace std;

//  namespace Reinstall {

// 	Builder::Builder(const Udjat::Properties &node) : Reinstall::Action{node}, output{Dialog::Factory("select-device",node)} {

// 		{
// 			// Search for EFI Boot definitions
// 			node.for_each("efi-boot-image",[this](const Properties &child) {
// 				boot.efi = make_shared<EFIBootImage>(child);
// 				return true; // Stop searching.
// 			});

// 			if(!boot.efi) {
// 				Logger::String{"Using default EFI Boot image"}.trace(name());
// 				boot.efi = make_shared<EFIBootImage>();
// 			} else {
// 				Logger::String{"Using customized EFI Boot image"}.trace(name());
// 			}
// 		}

// 		boot.theme = node["boot-theme"].as_quark();

// 		static const char *labels[] = {
// 			"grub-label",
// 			"boot-label",
// 			"label",
// 			"system-name",
// 		};

// 		for(const char *label : labels) {
// 			const char *ptr = String{node,label}.as_quark();
// 			if(ptr && *ptr) {
// 				boot.label = ptr;
// 				Logger::String{"Setting boot-label to '",boot.label,"' from attribute '",label,"'"}.trace(name());
// 				break;
// 			}
// 		}

// 		if(!(boot.label && *boot.label)) {
// 			String label{Config::Value<string>{"defaults","boot-label",_("Reinstall workstation")}.c_str()};
// 			label.expand(node);
// 			boot.label = label.as_quark();
// 			Logger::String{"Required attribute 'boot-label' is missing or invalid, using default '",boot.label,"'"}.warning(name());
// 		}

// 		// Load sources.
// 		Reinstall::DataSource::load(node,sources);

// 		// Load templates
// 		Reinstall::Template::load(*this,node,templates);

// 		// Load kernel parameters.
// 		Reinstall::KernelParameter::load(node,kparms);

// 	}

// 	Builder::~Builder() {
// 	}

// 	std::shared_ptr<Reinstall::Template> Builder::tmplt(const char *filename) {

// 		for(auto &tmplt : templates) {
// 			if(*tmplt == filename) {
// 				return tmplt;
// 			}
// 		}

// 		return std::shared_ptr<Reinstall::Template>();
// 	}

// 	void Builder::push_back(std::list<std::shared_ptr<DataSource>> &files, std::shared_ptr<DataSource> value) {

// 		class TemplateSource : public DataSource {
// 		private:
// 			const Udjat::Abstract::Object *parent;	///< @brief Parent object (for properties).
// 			std::shared_ptr<Reinstall::Template> tmplt;

// 			std::string tempfile;	///< @brief The temporary file with template applyed.

// 			struct {
// 				std::string local;
// 				std::string remote;
// 			} path;

// 		public:

// #ifdef BUILD_LEGACY
// 			TemplateSource(const Udjat::Abstract::Object &p, std::shared_ptr<Reinstall::Template> t, std::shared_ptr<DataSource> source)
// 				: DataSource{*source},parent{&p},tmplt{t} {

// 				path.local = source->local();
// 				path.remote = source->remote();

// 			}
// #else
// 			TemplateSource(const Udjat::Abstract::Object &p, std::shared_ptr<Reinstall::Template> t, std::shared_ptr<DataSource> source) 
// 				: DataSource{*source} {

// 				parent = &p;
// 				tmplt = t;

// 				path.local = source->local();
// 				path.remote = source->remote();

// 			}
// #endif // BUILD_LEGACY

// 			~TemplateSource() {
// #ifndef DEBUG
// 				if(!tempfile.empty()) {
// 					unlink(tempfile.c_str());
// 				}
// #endif // DEBUG
// 			}

// 			/// @brief Get URL for source on local filesystem.
// 			const char * local() const override {
// 				return path.local.c_str();
// 			}

// 			/// @brief Get URL for source on remote filesystem.
// 			const char * remote() const override {
// 				return path.remote.c_str();
// 			}

// 			std::string save(const Udjat::Abstract::Object &) override {
// 				// Apply template to temporary file.
// 				if(tempfile.empty()) {
// 					tempfile = Udjat::File::Temporary::create();
// 					debug("Applying template '",tmplt->name(),"' to temporary file ",tempfile.c_str());
// 					save(tempfile.c_str());
// 				}
// 				return tempfile.c_str();
// 			}

// 			void save(const char *path) override {
// 				debug("Applying template '",tmplt->name(),"' to file ",path);
// 				auto progress = ProgressFactory();		
// 				tmplt->save(*parent,path,[progress](uint64_t current, uint64_t total){
// 					progress->set(current,total);
// 					return false;
// 				});
// 				progress->done();
// 			}

// 		};

// 		// Check for template.
// 		for(auto &tmplt : templates) {
// 			const char *remote = value->remote();
// 			if(*tmplt == remote) {
// 				Logger::String{"Using template '",tmplt->name(),"' for ",remote}.trace(name());
// 				files.push_back(make_shared<TemplateSource>(*this,tmplt,value));
// 				return;
// 			}
// 		}

// 		// debug("Adding '",value->name(),"' data source with path ",value->path());
// 		files.push_back(value);
// 	}

// 	bool Builder::getProperty(const char *key, std::string &value) const {

// 		if(boot.label && *boot.label && !(strcasecmp(key,"boot-label") && strcasecmp(key,"install-label"))) {
// 			value = boot.label;
// 			return true;
// 		}

// 		if(!strcasecmp(key,"kernel-parameters")) {
// 			value = KernelParameter::join(*this,kparms);
// 			debug("Kernel parameters set to '",value.c_str(),"'");
// 			return true;
// 		}

// 		if(!strcasecmp(key,"boot-theme")) {

// 			if(boot.theme.empty()) {

// 				for(auto source : sources) {

// 					if(strncmp(source->path(),"/boot/",6)) {
// 						source->for_each([&](const char *filename){

// 							if(strncmp(filename,"./boot/",7)) {
// 								return false;
// 							}

// 							filename = strchr(filename+7,'/');
// 							if(!filename) {
// 								return false;
// 							}

// 							filename = strchr(filename+1,'/');
// 							if(!filename || strncmp(filename,"/themes/",8)) {
// 								return false;
// 							}

// 							filename += 8;
// 							const char *ptr = strchr(filename,'/');
// 							if(!ptr) {
// 								return false;
// 							}

// 							const_cast<Builder *>(this)->boot.theme = string{filename,(size_t)(ptr - filename)}.c_str();
// 							return true;

// 						});
// 						break;
// 					}
// 				}

// 				Logger::String{"Detected boot theme was '",boot.theme.c_str(),"'"}.trace(name());

// 			}

// 			value = boot.theme;
// 			return !value.empty();

// 		}

// 		value = Config::Value<string>{"defaults",key,""};
// 		if(!value.empty()) {
// 			return true;
// 		}

// 		return Reinstall::Action::getProperty(key,value);

// 	}

// 	void Builder::prepare(list<std::shared_ptr<DataSource>> &files) {

// 		Udjat::Dialog::Status::getInstance().sub_title(_("Getting required files"));

// 		for(auto &source : sources) {

// 			if(source->dir()) {

// 				// It's a directory, push back children
// 				source->for_each([this,&files](std::shared_ptr<DataSource> value){
// 					push_back(files,value);
// 					return false;
// 				});

// 			} else if(source->has_local()) {

// 				// It's a single file
// 				push_back(files,source);

// 			} else {

// 				// Single file without local path, insert a tempfile source.
// 				push_back(files,std::make_shared<TempFileSource>(*source));

// 			}
// 		}

// 		if(!files.size()) {
// 			throw runtime_error( _("Cant find installation files"));
// 		}

// 		Logger::String{files.size()," files to download"}.trace(name());

// 	}

//  }

