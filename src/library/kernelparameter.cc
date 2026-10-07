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
  * @brief Implements kernel parameters.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/tools/properties.h>
 #include <udjat/tools/object.h>
 #include <udjat/tools/string.h>
 #include <udjat/tools/intl.h>
 #include <udjat/tools/configuration.h>
 #include <vector>
 #include <unordered_map>
 #include <udjat/tools/quark.h>
 #include <linux/limits.h>
 #include <mntent.h>
 #include <sys/stat.h>
 #include <cstdio>
 #include <reinstall/tools/kernelparameter.h>
 #include <reinstall/tools/repository.h>

 using namespace std;
 using namespace Udjat;

 namespace IsoWriter {

	class UDJAT_PRIVATE KParm : public KernelParameter {
	private:
		const char *val;

	public:
		KParm(const char *name, const char *value) : KernelParameter{name}, val{value} {
		}

		KParm(const Udjat::Properties &node) : KernelParameter{node}, val{expand(node,"value")} {
		}

		std::string value(const Udjat::Abstract::Object &object) const override {
			return KernelParameter::value(object,val);
		}

	};

	std::vector<KernelParameter::Preset> KernelParameter::presets;

	KernelParameter::Preset::Preset(const char *n, const char *v)
		: name{Udjat::Quark{n}.c_str()}, value{Quark{v}.c_str()} {
	}

	KernelParameter::~KernelParameter() {
	}

	const char * KernelParameter::expand(const Udjat::Properties &node, const char *attrname) {
		return String{node,attrname}.expand(node).expand().as_quark();
	}

	std::string KernelParameter::value(const Udjat::Abstract::Object &object, const char *str) {
		String expanded{str};
		expanded.expand(object);
		return expanded;
	}

	String PathFactory(const Udjat::Properties &node, bool relative) {
		
		String path{node,"path"};
		if(path.empty() || !relative) {
			return path;
		}

		// Make path relative to partition.
		string file;
		string dir{path.c_str()};
		{
			auto pos = dir.find_last_of('/');
			if(pos == string::npos) {
				throw runtime_error(
					Logger::Message{
						_("Unable to get file mount without dirname for '{}' "),dir.c_str()
					}
				);
			}
			dir = dir.substr(0,pos+1);
			file = path.c_str() + pos + 1;
		}

		debug("dir='",dir.c_str(),"' name='",file.c_str(),"'");

		struct stat st;
		if(stat(dir.c_str(),&st)) {
			throw system_error(
				errno, 
				system_category(), 
				Logger::Message{
					_("Error getting status of '{}'"),dir.c_str()
				}
			);
		}

		if((st.st_mode & S_IFMT) != S_IFDIR) {
			throw runtime_error(
				Logger::Message{
					_("Unable to get mount point for '{}'"),dir.c_str()
				}
			);
		}
		struct mntent mnt;

		char buf[PATH_MAX*3];
	
		FILE *fp;
		struct mntent *fs;
		fp = setmntent("/etc/mtab", "r");
		while ((fs = getmntent_r(fp,&mnt,buf,sizeof(buf))) != NULL) {
			debug(fs->mnt_dir);
			struct stat stm;
			if(stat(fs->mnt_dir,&stm) == 0 && stm.st_dev == st.st_dev) {
				if((fs->mnt_dir[0] == '/' && fs->mnt_dir[1] == 0)) {
					Logger::String{"Mount point for '",path.c_str(),"' is root, no relocation is required"}.trace();
				} else {
					path = path.c_str() + strlen(fs->mnt_dir);
					Logger::String{"Mount point for '",dir.c_str(),"' is '",fs->mnt_dir,"' relative path is '",path.c_str(),"'"}.trace();	
				}
				break;
			}

		}
		endmntent(fp);

		return path;
	}

	void KernelParameter::load(const Udjat::Properties &node, std::vector<std::shared_ptr<KernelParameter>> &kparms, bool relpaths) {

		// Use map to avoid add of the same key more than one time.
		std::unordered_map<std::string, std::shared_ptr<KernelParameter>> keys;

		if(kparms.empty()) {

			for(const auto &value : presets) {

				if(keys.find(value.name) == keys.end()) {
					auto kparm = make_shared<KParm>(value.name,value.value);
					keys[value.name] = kparm;
					kparms.push_back(kparm);
				}

			}

		} else {

			for(auto kparm : kparms) {
				keys[kparm->parameter_name()] = kparm;
			}

		}

		// First search for 'kernel-parameter' nodes.
		node.for_each("kernel-parameter", [&](const Udjat::Properties &child) {

			if(child.contains("repository")) {

				throw runtime_error(
					Logger::Message{
						_("Kernel parameter '{}' has a 'repository' attribute, this is not allowed, use a 'repository' node instead"),child["name"].c_str()
					}
				);
	
			} else {

				// It's a standard kernel parameter.
				auto kparm = make_shared<KParm>(child);
				if(keys.find(kparm->parameter_name()) == keys.end()) {
					keys[kparm->parameter_name()] = kparm;
					kparms.push_back(kparm);
				}

			}

			return false; // Continue enumeration.

		});

		// Then search for repositories.
		node.for_each("repository", [&](const Udjat::Properties &child) {

			auto kparm = Repository::Factory(child);
			auto name = kparm->parameter_name();

			if(name && *name && keys.find(name) == keys.end()) {
				keys[name] = kparm;
				kparms.push_back(kparm);
			}

			return false; // Continue enumeration.

		});

		// Then search for driver-update-disks.
		node.for_each("driver-update-disk", [&](const Udjat::Properties &child) {

			auto name = child["kernel-parameter"];
			if(name.empty()) {
				name = Config::Value<string>("kernel-parameters","driver-update-disk","dud").c_str();
			}

			// Check if path is defined.
			{
				String path{PathFactory(child, relpaths)};
				if(!path.empty()) {
					// Local path is defined, use it.
					kparms.push_back(
						make_shared<KParm>(
							name.as_quark(),
							String{"hd:",path.c_str()}.as_quark()
						)
					);
					return false; // Continue enumeration.
				}
			}

			String url = child["url"];
			if(url.empty()) {
				throw runtime_error(Logger::Message{_("Driver update disk '{}' has no path or url defined"),String{child,"name"}.c_str()});
			}

			// Remote path is defined, use it.
			if(url[0] != '.') {

				// It's a full URL, use it without adjustments.
				kparms.push_back(
					make_shared<KParm>(
						name.as_quark(),
						url.as_quark()
					)
				);

			} else {

				// It's relative to repository, get value later.
				class RemoteUpdateDisk : public KernelParameter {
				private:
					std::shared_ptr<Repository> repository;
					const char *path;

				public:
					RemoteUpdateDisk(const char *name, std::shared_ptr<Repository> repo, const char *url) : KernelParameter{name}, repository{repo}, path{url} {
					}

					std::string value(const Udjat::Abstract::Object &object) const override {
						URL url{repository->remote()};
						url += path;
						url.expand(object);
						return url;
					}
			
				};

				kparms.push_back(
					make_shared<RemoteUpdateDisk>(
						name.as_quark(),
						Repository::Factory(child),
						url.as_quark()
					)
				);

			}

			return false; // Continue enumeration.
		});

	}

	std::string KernelParameter::join(const Udjat::Abstract::Object &object, const std::vector<std::shared_ptr<KernelParameter>> &kparms) {

		std::string result;

		for(auto kparm : kparms) {

			if(!result.empty()) {
				result += " ";
			}

			result += kparm->parameter_name();
			result += "=";

			string value = kparm->value(object);
			if(value.empty()) {
				throw logic_error(Logger::Message{_("Kernel parameter '{}' has an empty value"),kparm->parameter_name()});
			}
			result += value;

		}

		return result;
	}

	void KernelParameter::preset(const char *arg) {
		const char *ptr = strchr(arg,'=');

		if(!ptr) {
			ptr = strchr(arg,':');
		}

		if(!ptr) {
			throw runtime_error("Invalid kernel parameter definition");
		}

		preset(string{arg,(size_t) (ptr-arg)}.c_str(),ptr+1);
	}

 }

