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

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/tools/logger.h>
 #include <reinstall/action.h>
 #include <reinstall/dialog.h>
 #include <udjat/tools/activatable.h>
 #include <udjat/tools/intl.h>
 #include <udjat/tools/application.h>

 using namespace Udjat;

 namespace Reinstall {

	Action::Action(const Udjat::Properties &props) : String{props["name"].c_str()} {

		if(props.contains("model")) {

			auto name = props["model"];
			Logger::String{"Building action using model '", name.c_str(),"' for node '",props.path(),"'"}.trace(this->c_str());

	#ifdef DEBUG
			String path{getenv("PWD")};
			path += "/models/";
	#else
			String path{Application::DataDir{"models"}.c_str()};
	#endif // DEBUG

			path += name;

			debug("Model path: ",path.c_str());

			const_cast<Udjat::Properties &>(props).load(path.c_str());

		}

		confirmation = Dialog::Factory("confirmation",props,_("Do you confirm?"));
		success = Dialog::Factory("success",props,_("Operation complete"),Dialog::QuitContinue);
		failed = Dialog::Factory("failed",props,_("Operation failed"),Dialog::QuitContinue);

	}

	Action::~Action() {

	}

	/// @brief Test if the action is valid and can be activated.
	bool Action::initialize() {
		return true;
	}

	void Action::activate() {
		
	}


 }

//  /**
//   * @brief Implements abstract action.
//   */

//  #include <config.h>
//  #include <udjat/defs.h>
//  #include <reinstall/action.h>
//  #include <reinstall/dialog.h>
//  #include <udjat/tools/object.h>
//  #include <udjat/tools/intl.h>
//  #include <udjat/tools/application.h>
//  #include <stdexcept>
//  #include <reinstall/dialog.h>
//  #include <udjat/tools/intl.h>
//  #include <udjat/tools/properties.h>

//  using namespace Udjat;
//  using namespace std;

//  namespace Reinstall {

// 	const char * Action::presets[2] = {nullptr,nullptr};

// 	Model::Model(const Udjat::Properties &node) {

// 		auto logname = node["name"];

// 		String name{node,"model"};
// 		if(name.empty()) {
// 			Logger::String{"Building action for node '",node.path(),"'"}.info(logname.c_str());
// 			return;
// 		}

// 		Logger::String{"Building action using model '", name.c_str(),"' for node '",node.path(),"'"}.trace(logname.c_str());

// #ifdef DEBUG
// 		String path{getenv("PWD")};
// 		path += "/models/";
// #else
// 		String path{Application::DataDir{"models"}.c_str()};
// #endif // DEBUG

// 		path += name;

// 		debug("Model path: ",path.c_str());

// 		const_cast<Udjat::Properties &>(node).load(path.c_str());

// 	}

// 	Action::Action::Action(const Udjat::Properties &node) 
// 		: Model{node}, NamedObject{node}, 
// 			dialog_title{node["dialog-title"].as_quark()},
// 		 	icon_name{node["icon-name"].as_quark()} {
		
// 		if(!(dialog_title && *dialog_title)) {
// 			dialog_title = node["title"].as_quark();
// 		}

// 		if(!(icon_name && *icon_name)) {
// 			icon_name = node["icon"].as_quark();
// 		}	

// 		confirmation = Dialog::Factory("confirmation",node,_("Do you confirm?"));
// 		success = Dialog::Factory("success",node,_("Operation complete"),Dialog::QuitContinue);
// 		failed = Dialog::Factory("failed",node,_("Operation failed"),Dialog::QuitContinue);

// 	}

// 	Action::~Action() {

// 	}

// 	/// @brief Activate the action, called on selected action when the 'apply' button is pressed.
// 	void Action::activate() {
// 		Logger::String{"This action cant be activated"}.error(name());	
// 		throw logic_error(_("The selected action cant be activated"));
// 	}

// 	/// @brief Test if the action is valid and can be activated.
// 	bool Action::initialize() {
// 		return true;
// 	}

// 	void Action::preset(const char *value) {

// 		auto args = Udjat::String{value}.split("/",2);
// 		if(args.size() != 2) {
// 			throw std::runtime_error{Logger::Message(_("Unable to select '{}': Invalid path"),value)};
// 		}

// 		presets[0] = args[0].as_quark();
// 		presets[1] = args[1].as_quark();
		
// 		Logger::String{"Setting preset to '",presets[0],"/",presets[1],"'"}.info();
// 	}

// 	bool Action::is_default(const Udjat::Properties &node) noexcept {

// 		bool has_preset = Action::has_preset();
// 		if(has_preset) {

// 			if(strcasecmp(presets[0],node.parent().get("name","default").c_str())) {
// 				return false;
// 			}

// 			if(strcasecmp(presets[1],node.get("name","default").c_str())) {
// 				return false;
// 			}	

// 		}

// 		return node.get("default",has_preset) || 
// 		       node.get("selected",has_preset) || 
// 		       node.get("active",has_preset);

// 	}

//  }
 
