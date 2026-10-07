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
  * @brief Implements script.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/tools/properties.h>
 #include <udjat/tools/object.h>
 #include <udjat/tools/url.h>
 #include <udjat/tools/string.h>
 #include <udjat/tools/file/text.h>
 #include <udjat/tools/file/temporary.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/tools/subprocess.h>
 #include <reinstall/tools/datasource.h>
 #include <reinstall/tools/script.h>
 #include <reinstall/tools/repository.h>
 #include <udjat/tools/intl.h>
 #include <stdexcept>
 #include <pwd.h>
 #include <grp.h>

 #ifdef HAVE_UNISTD_H
	#include <unistd.h>
 #endif // HAVE_UNISTD_H

 using namespace Udjat;
 using namespace std;

 namespace IsoWriter {

 	static int getuid(const Properties &node) {

 		auto user = node["user"];

 		if(user.empty()) {
			return -1;
 		}

		size_t szBuffer = sysconf(_SC_GETPW_R_SIZE_MAX);
		if(szBuffer == (size_t) -1) {
			szBuffer = 16384;
		}

		char buffer[szBuffer+1];
		memset(buffer,0,szBuffer+1);

		struct passwd pwd;
		struct passwd *result;

		if(getpwnam_r(user.c_str(), &pwd, buffer, szBuffer, &result) != 0) {
			throw system_error(errno,system_category(),user);
		};

		if(!result) {
			throw system_error(ENOENT,system_category(),user);
		}

		return (int) result->pw_uid;

 	}

 	static int getgid(const Properties &node) {

 		auto group = node["group"];

 		if(group.empty()) {
			return -1;
 		}

		size_t szBuffer = sysconf(_SC_GETPW_R_SIZE_MAX);
		if(szBuffer == (size_t) -1) {
			szBuffer = 16384;
		}

		char buffer[szBuffer+1];
		memset(buffer,0,szBuffer+1);

		struct group grp;
		struct group *result;

		if(getgrnam_r(group.c_str(), &grp, buffer, szBuffer, &result) != 0) {
			throw system_error(errno,system_category(),group);
		};

		if(!result) {
			throw system_error(ENOENT,system_category(),group);
		}

		return (int) result->gr_gid;

 	}

	Script::Script(const Udjat::Abstract::Object &parent, const Udjat::Properties &node)
		: IsoWriter::FileSource{node,false}, rtime{(Script::RunTime) node.get("type","post").select("pre","post",nullptr)},
		marker{node.get("marker",Config::Value<String>("marker","$").c_str())[0]},
		uid{getuid(node)}, gid{getgid(node)}, cmdline{String{node,"cmdline"}.as_quark()} {

		// Just in case.
		Udjat::NamedObject::rename(String{node,"name","unnamed"}.as_quark());

		if(cmdline && *cmdline) {
			
			Logger::String{"Using command line '",cmdline,"'"}.trace(name());
		
		} else {

			String text{node.child_value()};
			text.strip();

			if(text.empty()) {

				// Not text, get from URLs.

				URL attr{node["url"].c_str()};

				url.remote = node["remote"].as_quark();
				url.local = node["local"].as_quark();

				if(!url.local[0] && attr.local()) {
					url.local = attr.as_quark();
					if(url.local[0]) {
						Logger::String{"Will get local script from",url.local}.trace(name());
					}
				}

				if(!(url.remote[0] || attr.remote())) {
					url.remote = attr.as_quark();
					if(url.remote[0]) {
						Logger::String{"Will get remote script from ",url.remote}.trace(name());
					}
				}

				if(!(url.local[0] || url.remote[0])) {
					throw runtime_error(Logger::String{"Required attribute 'url' is missing or invalid on ",node.path()});
				}

			} else {

				// Has script code.
				Logger::String{"Using script from XML node"}.trace(name());

				if(marker) {
					text.expand(marker,parent);
					text.expand(marker,node);
				}

				if(node.get("strip-lines",false)) {
					String stripped;
					text.for_each("\n",[&stripped](const String &value) {
						stripped += const_cast<String &>(value).strip();
						stripped += "\n";
						return false;
					});
					this->code = stripped.as_quark();

				} else {
					this->code = text.as_quark();
				}

				if(Logger::enabled(Logger::Debug)) {
					Logger::String{"Parsed script:\n",this->code}.write(Logger::Debug,name());
				}

			}

			if(url.remote[0] == '.' || url.local[0] == '.' || url.remote[0] == '/' || url.local[0] == '/') {
				repository = Repository::Factory(node);
			}

		}


	}

	Script::~Script() {
		if(!tempfilename.empty()) {
			unlink(tempfilename.c_str());
		}
	}

	void Script::load(const Udjat::Abstract::Object &parent, const Udjat::Properties &node, std::vector<std::shared_ptr<Script>> &scripts) {

		node.for_each_child("script", [&scripts, &parent](const Udjat::Properties &child){
			scripts.push_back(make_shared<Script>(parent,child));
			return false;
		});

		debug("Got ",scripts.size()," scripts");

	}

	void Script::run(const Udjat::Abstract::Object &object, const RunTime rtime, const char *msg) {

		class SubProcess : public Udjat::SubProcess {
		private:
			int uid = -1;
			int gid = -1;

		protected:

			void pre() override {
				if(uid != -1 && setuid(uid) != 0) {
#ifdef DEBUG
					Logger::String{"Cant set subprocess user id"}.error("subprocess");
#else
					throw system_error(errno,system_category(),"Cant set subprocess user id");
#endif // DEBUG
				}
				if(gid != -1 && setgid(gid) != 0) {
#ifdef DEBUG
					Logger::String{"Cant set subprocess group id"}.error("subprocess");
#else
					throw system_error(errno,system_category(),"Cant set subprocess group id");
#endif // DEBUG
				}
			}

		public:
			SubProcess(int u, int g, const char *name, const Udjat::String &command)
				: Udjat::SubProcess{name,command.c_str(),Logger::Info,Logger::Error}, uid{u}, gid{g} {
			}
		};
		
		if(rtime != this->rtime) {
			return;
		}

		String text;

		if(code && *code) {

			text = code;

		} else if(cmdline[0]) {

			// Execute command line.
			String cmd{cmdline};
			cmd.expand(object);
			cmd.expand(*this);
	
			debug("running '",cmd.c_str(),"'");

			int rc = SubProcess{
				uid,
				gid,
				object.name(),
				cmd
			}.run();

			if(rc) {
				throw runtime_error(_("External command failed"));
			}
			
			return;
			
		} else {

			std::string filename;

			if(url.remote && *url.remote) {

				Dialog::Progress::getInstance()->url(url.remote);

				if(url.local && *url.local) {

					// Save to local path.
					filename = this->local();
					DataSource::save(filename.c_str());

				} else if(tempfilename.empty()) {

					// Save to temporary path.
					filename = tempfilename = File::Temporary::create();
					DataSource::save(filename.c_str());

				} else {

					filename = tempfilename;

				}

			} else if(url.local && *url.local) {

				filename = Udjat::URL{url.local}.path();

			} else {

				throw runtime_error("Unable to determine script filename");

			}

			debug("loading ",filename.c_str());
			text = Udjat::File::Text{filename.c_str()}.c_str();

		}

		text.expand(marker,object);
		text.expand(marker,*this);

		auto script	= File::Temporary::create();

		try {

			{
				File::Handler out{script.c_str(),true};
				out.truncate();
				out.write(text.c_str(),text.size());
			}

			chmod(script.c_str(),0755);

			auto progress = Udjat::Dialog::Progress::getInstance();
			if(message && *message) {
				progress->url(message);
			} else if(msg && *msg) {
				progress->url(msg);
			} else {
				progress->url(_("Configuring, please wait..."));
			}

			SubProcess{uid,gid,object.name(),script}.run();

		} catch(...) {

#ifndef DEBUG
			unlink(script.c_str());
#endif // DEBUG
			throw;

		}

#ifndef DEBUG
		unlink(script.c_str());
#endif // DEBUG

	}

 }


