/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Perry Werneck <perry.werneck@gmail.com>
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
  * @brief Implements gtk4 toplevel.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <vector>
 #include <private/application.h>
 #include <iostream>
 #include <reinstall/group.h>
 #include <udjat/ui/menu.h>
 #include <udjat/ui/console/menu.h>
 #include <udjat/tools/intl.h>

 #ifdef LOG_DOMAIN
	#undef LOG_DOMAIN
 #endif
 #define LOG_DOMAIN "tui"
 #include <udjat/tools/logger.h>

 using namespace Udjat;
 using namespace std; 

 namespace IsoWriter {

	int Application::run_tui() {

		class TextApplication : public IsoWriter::Application {
		private:

			class Group : public IsoWriter::Group {
			private:
				std::string title;
				std::vector<std::shared_ptr<Action>> actions;

			public:
				Group(const Udjat::Properties &props) 
					: IsoWriter::Group{props},title{props["title"].c_str()} {
				}

				const char *label() const noexcept override {
					return title.c_str();
				}

				void push_back(const Udjat::Properties &,std::shared_ptr<Action> action) override {
					actions.push_back(action);
				}

			};

		public:

			TextApplication() {
			}

			int run_interactive() override {

				while(1) {

					Console::Menu<string> menu{_("Select system:")};
					for(const auto &group: groups) {
						menu.append(group->label());
					}

					try {

						auto group = groups[menu.select()];
						Logger::String{"User selected '",group->label(),"'"}.info();

					} catch(const std::exception &e) {
						Logger::String{e.what()}.info();
						return 0;
					}

					
				}

				return 0;
			}

			int run_non_interactive() override {

				return -1;
			}

			bool build(const Udjat::Properties &props) override {
				push_back(props,make_shared<Group>(props));
				return true;
			}

		};

		return TextApplication{}.run();
	}

 }
 