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
 #include <udjat/tools/configuration.h>

 #include <reinstall/tools/datasource.h>
 #include <reinstall/tools/repository.h>
 #include <private/slpclient.h>
 #include <list>

 #ifdef HAVE_ZLIB
	#include <zlib.h>
 #endif // HAVE_ZLIB

 using namespace Udjat;
 using namespace std;

 namespace IsoWriter {

	Repository::KParm::KParm(const Udjat::Properties &node) {

		String slpkparm;
		String logname{node,"name"};

		// TODO: Refactor this to use string{node,attribute-name}.
		for(auto child = node.child("attribute");child;child = child.next_sibling("attribute")) {
			if(!strcasecmp(child.attribute("name").as_string("none"),"kernel-parameter-name")) {
				name = String{child,"value"}.as_quark();
				slpkparm = String{child,"slp-value"};
				enabled = child.attribute("allow-slp").as_bool(!slpkparm.empty());
			}
		}

		if(slpkparm.empty() && Config::Value<bool>{"application","legacy",true}) {
			slpkparm = String{node,"slp-kernel-parameter"};
			if(!slpkparm.empty()) {
				Logger::String{"Got slp kernel parameter using legacy attribute 'slp-kernel-parameter' at ",node.path()}.trace(logname.c_str());
				enabled = true;
			}
		}

		// Apply quirks on SLP kernel parameter.
		{
			// Check for quirks
			String quirk{node,"slp-value-quirk"};
			if(!quirk.empty()) {
				Config::Value<string> qvalue{"quirks",quirk.c_str()};
				if(qvalue.empty()) {
					Logger::String{"Unknown quirk '",quirk.c_str(),"' for slp kernel parameter"}.warning(logname.c_str());
				} else {
					auto values = String{qvalue.c_str()}.split(",");
					Logger::String{"Applying quirk ",quirk.c_str(),": '",values[0].c_str(),"' -> '",values[1].c_str(),"' for slp kernel parameter at ",node.path()}.trace(logname.c_str());
					for(auto pos = slpkparm.find(values[0].c_str()); pos != std::string::npos; pos = slpkparm.find(values[0].c_str(), pos + values[1].size())) {
						slpkparm.replace(pos, values[0].size(), values[1].c_str());
					}
				}
			} else {
				Logger::String{"No quirk for slp kernel parameter at ",node.path()}.trace(logname.c_str());
			}
		}

		// Set converted slp value.
		Logger::String{"SLP kernel parameter set to '",slpkparm.c_str(),"'"}.trace(logname.c_str());	
		slp = slpkparm.as_quark();
	}

 }


