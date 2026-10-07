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
  * @brief Implements templates.
  */

 #define LOG_DOMAIN "template"
 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/template.h>
 #include <reinstall/tools/template.h>
 #include <udjat/tools/properties.h>
 #include <reinstall/tools/datasource.h>
 #include <udjat/tools/intl.h>
 #include <iostream>
 #include <fstream>
 #include <sstream>
 #include <filesystem>
 #include <udjat/tools/http/mimetype.h>

 using namespace Udjat;
 using namespace std;

 namespace Reinstall {

	Template::Template(const Udjat::Properties &props) 
        : std::string{props["name"].c_str()}, Udjat::Template{props,"file"} {

        this->path = props["path"];
    }

	Template::Template(const char *name, const char *path) : std::string{name}, Udjat::Template{name,Udjat::MimeType::none} {
        this->path = path;
    }

    // void Template::apply(std::ostream &out) {
    //     apply(out,[](const char *key, std::ostream &stream){
    //         debug("key='",key,"'");

    //         return false;
    //     });
    // }

	void Template::load(const std::function<bool(uint64_t current, const void *buf, size_t length)> &writer) {
        stringstream out;
        this->apply(out);
        auto text = out.str();
        writer(0,text.c_str(),text.size());
    }

	std::string Template::save() {        
        auto filename = this->filename();
        std::filesystem::resize_file(filename, 0); 

        std::ofstream out{filename};
        if (!out.is_open()) {
            throw runtime_error(_("Error opening output file"));
        }
        apply(out);
        out.close(); 

        return filename;
    }

 }

//  #include <config.h>
//  #include <udjat/defs.h>
//  #include <udjat/tools/properties.h>
//  #include <udjat/tools/object.h>
//  #include <reinstall/tools/template.h>
//  #include <udjat/tools/application.h>
//  #include <udjat/tools/configuration.h>
//  #include <udjat/tools/string.h>
//  #include <udjat/tools/quark.h>
//  #include <udjat/tools/url.h>
//  #include <udjat/tools/file/text.h>
//  #include <udjat/tools/file/path.h>
//  #include <udjat/tools/logger.h>
//  #include <udjat/tools/intl.h>
//  #include <udjat/tools/url/handler.h>
//  #include <stdexcept>

//  #include <unistd.h>

//  using namespace Udjat;
//  using namespace std;

//  namespace Reinstall {

// 	Template::Template(const Udjat::Properties &node)
// 		: Udjat::NamedObject{node}, 
// 			escape{node.get("escape-control-characters",false)},
// 			quirk{node["quirk"].as_quark()} {

// 		// Get marker.
// 		{
// 			auto sMarker = node.get(
// 									"marker",
// 									Config::Value<String>("template","marker","$").c_str()
// 								);

// 			if(sMarker.size() > 1 || sMarker.empty()) {
// 				throw runtime_error("Marker attribute is invalid");
// 			}

// 			this->marker = sMarker[0];
// 		}

// 		// Get Type
// 		if(node.get("binary",false)) {
// 			type = (Type) (type|Template::Binary);
// 		} else {
// 			type = (Type) (type|Template::Text);
// 		}

// 		if(node.get("script",false)) {
// 			Logger::String{node.path()," is using deprecated attribute 'script', use 'executable' instead"}.trace(name());
// 			mode = 0755;
// 		}

// 		if(node.get("executable",false)) {
// 			mode = 0755;
// 		}

// 		// Get URL
// 		{
// 			auto str = node["url"];
// 			// debug("Raw template URL from XML is '",node.attribute("url").as_string(),"'");
// 			if(str.empty()) {
// 				throw runtime_error(Logger::String{"Required attribute 'url' is missing or invalid on ",node.path()});
// 			}
// 			debug("Raw template URL is '",str.c_str(),"'");

// 			str.unescape();
// 			debug("Unescaped template URL is '",str.c_str(),"'");

// 			str.expand(*this,false);
// 			debug("Expanded template URL is '",str.c_str(),"'");

// 			url = str.as_quark();

// 			debug("Template URL set to '",url,"'");

// 		}

// 		// Get path
// 		{
// 			auto str = node["path"];
// 			if(!str.empty()) {

// 				str.unescape();
// 				str.expand(*this);

// 				path = str.as_quark();

// 				debug("Template path set to '",url,"'");
// 			}


// 		}

// 	}

// 	Template::~Template() {

// 		if(!tempfilename.empty()) {
// 			unlink(tempfilename.c_str());
// 		}
// 	}

// 	bool Template::operator==(const char *path) const {

// 		const char *ptr = strrchr(path,'/');
// 		if(ptr && !strcmp(ptr+1,name())) {
// 			return true;
// 		}

// 		return false;
// 	}

// 	bool Template::getProperty(const char *key, std::string &value) const {

// 		debug("Getting template property '",key,"'");

// 		if(!strcasecmp(key,"template-dir")) {
// #ifdef DEBUG
// 			value = getenv("PWD");
// 			value += "/templates";
// #else
// 			value = Application::DataDir{"templates"};
// #endif // DEBUG
// 			debug("Template directory set to '",value.c_str(),"'");
// 			return true;
// 		}

// 		if(!strcasecmp(key,"models-dir")) {
// #ifdef DEBUG
// 			value = getenv("PWD");
// 			value += "/models";
// #else
// 			value = Application::DataDir{"models"};
// #endif // DEBUG
// 			return true;
// 		}

// 		return Udjat::NamedObject::getProperty(key,value);
// 	}

// 	void Template::load(const Udjat::Abstract::Object &parent, const Udjat::Properties &node, std::vector<std::shared_ptr<Template>> &templates) {

// 		node.for_each_child("template", [&templates](const Properties &child){
// 			templates.push_back(make_shared<Template>(child));
// 			return false;
// 		});

// 		debug("Got ",templates.size()," templates");

// 	}

// 	void Template::save(const Udjat::Abstract::Object &parent, const char *path, const std::function<bool(uint64_t current, uint64_t total)> &progress) {

// 		String filename{path};
// 		filename.expand(parent);
// 		filename.expand(*this);

// 		String text;

// 		// Download template
// 		{
// 			URL url{this->url};
// 			url.expand(parent);
// 			url.expand(*this);

// 			auto handler = url.handler();
// 			handler->update_if_exists(false); // Never cache template sources.
// 			text = handler->get(progress);
// 		}
		
// 		text.expand(marker,parent);
// 		text.expand(marker,*this);

// 		/*
// 		Udjat::URL url{this->url};
// 		url.expand(parent);
// 		url.expand(*this);

// 		String text{url.get(progress)};
// 		*/

// 		// handle escape sequences.
// 		if(escape) {
// 			static const struct {
// 				const char *from;
// 				const char *to;
// 			} escapes[] = {
// 				{"&","\\&"},
// 			};

// 			debug("Text before escapes:\n",text.c_str());
// 			for(const auto &e : escapes) {
// 				for(auto pos = text.find(e.from); pos != std::string::npos; pos = text.find(e.from, pos + strlen(e.to))) {
// 					text.replace(pos, strlen(e.from), e.to);
// 				}
// 			}
// 			debug("Text after escapes:\n",text.c_str());
// 		}

// 		if(quirk && *quirk) {
// 			Config::Value<string> qvalue{"quirks",quirk};
// 			if(qvalue.empty()) {
// 				Logger::String{"Unknown quirk '",quirk,"' on template"}.warning(name());
// 			} else {
// 				auto values = String{qvalue.c_str()}.split(",");
// 				Logger::String{"Applying quirk ",quirk,": '",values[0].c_str(),"' -> '",values[1].c_str(),"'"}.trace(name());
// 				for(auto pos = text.find(values[0].c_str()); pos != std::string::npos; pos = text.find(values[0].c_str(), pos + values[1].size())) {
// 					text.replace(pos, values[0].size(), values[1].c_str());
// 				}
// 			}
// 		}

// 		if(script) {

// 			// Execute script, use stdout to set template result.
// 			throw runtime_error("Script templates are not supported yet");

// 		}

// 		// and save parsed contents.
// 		{
// 			File::Handler out{filename.c_str(),true};
// 			out.truncate();
// 			out.write(text.c_str(),text.size());
// 		}

// 		if(Logger::enabled(Logger::Debug)) {
// 			Logger::String(text.c_str()).write(Logger::Debug,name());
// 		}

// 		if(chmod(filename.c_str(),mode) < 0) {
// 			throw system_error(errno,system_category(),_("Cant update template permissions"));
// 		}

// 		debug("Template '",name(),"' saved on file ",path);

// 	}

//  }

