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

 /**
  * @brief The reinstall application.
  */

 #include <config.h>
 #include <private/application.h>
 #include <udjat/tools/properties.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/configuration.h>
 #include <stdexcept>
 #include <vector>
 #include <memory>
 #include <udjat/tools/string.h>
 #include <udjat/tools/intl.h>
 #include <reinstall/group.h>
 #include <udjat/module/http.h>
 #include <udjat/ui/console/progress.h>
 #include <reinstall/progress.h>

 using namespace Udjat;
 using namespace std;

 namespace Reinstall {

	Application * Application::instance = nullptr;
	bool Application::non_interactive_mode = false;
	std::vector<String> Application::selected_path;

	Application::Application() : Properties::ObjectBuilder("group") {
		if(instance) {
			throw std::logic_error("Application instance already exists");
		}
		instance = this;

#ifdef STATIC_MODULES
		//
		// Load modules
		//
		{
#ifndef _WIN32
			// if(Config::Value<bool>{"modules","grub2",true}) {

			// 	Reinstall::Grub2::Module::Factory("grub");

			// 	if(Config::Value<bool>{"application","legacy",false}) {
			// 		Reinstall::Grub2::Module::Factory("grub");
			// 	}

			// }
#endif

			if(Config::Value<bool>{"modules","http",true}) {
				Logger::String{"Loading http module"}.info();
				Udjat::HTTP::Module::Factory();
			}

			// if(Config::Value<bool>{"modules","isowriter",true}) {
			// 	Logger::String{"Loading isowriter module"}.info();
			// 	Reinstall::IsoWriter::Module::Factory();
			// }

			// if(Config::Value<bool>{"modules","isobuilder",true}) {

			// 	Logger::String{"Loading isobuilder module"}.info();
			// 	Reinstall::IsoBuilder::Module::Factory();

			// 	if(Config::Value<bool>{"application","legacy",false}) {
			// 		Logger::String{"Loading network-installer module (legacy)"}.info();
			// 		Reinstall::IsoBuilder::Module::Factory("netinstall","network-installer");
			// 	}

			// }


		}		
#endif // STATIC_MODULES

	}

	Application::~Application() {
		instance = nullptr;
		Module::unload();
	}

	int Application::run() {

		//
		// Load options
		//
#ifdef DEBUG
		Properties::parse(MimeType::xml,"./xml.d");
#else
		Properties::parse(MimeType::xml);
#endif

		//
		// Run user interaction.
		//	
		if(non_interactive_mode) {
			return run_non_interactive();
		}

		return run_interactive();

	}

	Application & Application::get_instance() {
		if(!instance) {
			throw std::logic_error("Application instance does not exist");
		}
		return *instance;
	}

	void Application::set_selected_path(const char *path) {
		if(!(path && *path)) {
			throw std::invalid_argument("Missing path");
		}
		selected_path.clear();
		String{path}.split(selected_path,"/");
	}

	bool Application::push_back(const Udjat::Properties &props, std::shared_ptr<Group> group) {

		for(const auto &itn : groups) {
			if(!strcasecmp(itn->c_str(),group->c_str())) {
				Logger::String{"Group '", group->c_str(), "' already exists"}.error("groups");
				return false;
			}
		}

		groups.push_back(group);

		if(selected_path.size() >=1) {
			if(strcasecmp(selected_path[0].c_str(),group->c_str())) {
				return false;
			}
			Logger::String{"Auto-selecting group '",group->label(),"' by command-line path"}.info("groups");
			selected_group = group;
			return true;
		}

		if(props.get("default",false)) {
			Logger::String{"Auto-selecting group '",group->label(),"'"}.info("groups");
			selected_group = group;
			return true;
		}

		return false;
	}

	std::shared_ptr<Group> Application::find_group(const Udjat::Properties &props) {
	
		if(groups.empty()) {
			throw logic_error(_("A valid group is required to perform this action"));
		}

		if(!props.contains("group")) {
			return groups.back();
		}

		auto name = props["group"];
		for(const auto &itn : groups) {
			if(!strcasecmp(itn->c_str(),name.c_str())) {
				return itn;
			}
		}

		throw runtime_error(Logger::Message{_("Required group '{}' is undefined"),name.c_str()});

	}

	bool Application::push_back(const Udjat::Properties &props,std::shared_ptr<Action> action) {
		find_group(props)->push_back(props,action);
		return true;
	}

	int Application::run_interactive() {
		throw std::logic_error("Interactive mode is not implemented");
	}

	int Application::run_non_interactive() {
		throw std::logic_error("Non-interactive mode is not implemented");
	}

	std::shared_ptr<Progress> Application::ProgressDialogFactory() {
		return make_shared<Progress>();
	}

	std::shared_ptr<Progress> Progress::Factory() {
		return Application::get_instance().ProgressDialogFactory();
	}

	bool Application::build(const Udjat::Properties &props) {
		throw std::logic_error("Build method is not implemented");
	}

 }

 