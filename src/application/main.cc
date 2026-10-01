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
  * @brief Application entry point.
  */

 #ifdef HAVE_GTKMM
	#include <gtkmm.h>
 #endif
 
 #include <config.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/argumentparser.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/tools/intl.h>
 #include <udjat/tools/application.h>
 #include <cstdint>
 #include <iostream>

 #include <private/application.h>

#ifdef DEBUG
 #include <udjat/tools/testsuite.h>
 #include <udjat/module/http.h>
#endif // DEBUG

 #include <reinstall/tools/repository.h>

#ifdef HAVE_GTKMM
 static bool has_graphical_session();
#endif

 using namespace Reinstall;
 using namespace Udjat;
 using namespace std;

 enum UIMode : uint8_t {
	TUI,
#ifdef HAVE_GTKMM
	GUI,
#endif // HAVE_GTKMM
 }; 

#if defined(HAVE_GTKMM) && !defined(DEBUG)
 static UIMode ui_mode = GUI;
#else
 static UIMode ui_mode = TUI;
#endif

 int main(int argc, char **argv) {

#ifdef GETTEXT_PACKAGE
	// Set locale.
	Udjat::Application::set_gettext_package(GETTEXT_PACKAGE);
#endif // GETTEXT_PACKAGE

#ifdef DEBUG
	Udjat::Config::allow_user_homedir(true);
	Logger::verbosity(9);
	Logger::console(true);
#endif

	//
	// Check defaults
	//
	{
		const char *ptr = strrchr(argv[0],'/');
		if(ptr) {
			ptr++;
			if(strcmp(ptr,"reinstall-enable") == 0 || strcmp(ptr,"reinstall-system") == 0) {
				Reinstall::Application::non_interactive(true);
			}
		}

#ifdef HAVE_GTKMM
		if(!has_graphical_session()) {
			ui_mode = TUI;
		}
#endif

	}

	//
	// Parse arguments
	//
	{
		ArgumentParser parser;

		using Argument = ArgumentParser::Argument;
		using Result = ArgumentParser::Result;

		if(!Reinstall::Application::non_interactive()) {
			parser.append(
				Argument{
					'y', "non-interactive", _("Run in non-interactive mode"),
					[](const char *, char) {
						Reinstall::Application::non_interactive(true);
						return Result::Handled;
					}
				}
			);
		}

		parser.append(
			Argument{
				'O', "output", _("Write image to file instead of device"), _("filename"),
                [](const char *arg, char) {
					if(!arg || !*arg) {
						throw std::invalid_argument("Missing output filename");
					}
					return Result::Handled;
				}
			},
#ifdef DEBUG
			Argument{
				'i', "interactive-testcases", "Run testsuite",
                [](const char *arg, char) {

#ifdef STATIC_MODULES
				Logger::String{"Loading http module"}.info();
				Udjat::HTTP::Module::Factory();
#endif // STATIC_MODULES

					TestSuite testsuite;
					udjat_register_tests(testsuite);
					testsuite.load();

					if(arg && *arg) {
						testsuite.run(arg);
					} else {
						testsuite.interactive();
					}

					Udjat::Module::unload();

					return Result::ExitNow;
				}
			},
#endif // DEBUG
#ifdef HAVE_GTKMM
			Argument{
				't', "tui", _("Run in text mode"),
                [](const char *, char) {
					ui_mode = TUI;
					return Result::Handled;
				}
			},
			Argument{
				'g', "gui", _("Run in graphical mode"),
                [](const char *, char) {
					ui_mode = GUI;
					return Result::Handled;
				}
			},
#endif // HAVE_GTKMM
			_("Build options"),
			Argument{
				'S', "select", _("Auto-select image to build"), _("path"),
                [](const char *arg, char) {
					Reinstall::Application::set_selected_path(arg);
					return Result::Handled;
				}
			},
			Argument{
				'Q', "quit", _("Quit after processing"),
                [](const char *, char) {
					return Result::Handled;
				}
			},
			Argument{
				'R', "reboot", _("Reboot after processing"),
                [](const char *, char) {
					return Result::Handled;
				}
			},
			_("Network options"),
			Argument{
				'I', "install", _("Set URL for installation repository"), "url",
                [](const char *arg, char) {
					Repository::url(arg);
					return Result::Handled;
				}
			},
			Argument{
				'S', "server", _("Set hostname/ip for all repositories"), "url",
                [](const char *arg, char) {
					Repository::host(arg);
					return Result::Handled;
				}
			},
			Argument{
				'T', "target", _("Set installation repository from target-name"), "target",
                [](const char *arg, char) {
					return Result::Handled;
				}
			},
			Argument{
				'X', "no-slp", _("Disable SLP search"),
                [](const char *arg, char) {
					return Result::Handled;
				}
			}
			
		);

		try {

			if(parser.add_logger_group().parse(argc,argv)) {
				return 0;
			}

		} catch(const std::exception &e) {

			cerr << endl << e.what() << endl;
			return -1;
	
		}

	}

	//
	// Start application
	//
#ifdef HAVE_GTKMM
	if(ui_mode == GUI) {
		Logger::String{"Starting graphical mode"}.trace();
		return -1;
	}
#endif // HAVE_GTKMM

	Logger::String{"Starting text mode"}.trace();

	try {

		return Reinstall::Application::run_tui();

	} catch(const std::exception &e) {

		Logger::console(false);
		Logger::String{e.what()}.error("tui");

		cerr << endl << e.what() << endl;
	}
	
	return -1;

 }

#ifdef HAVE_GTKMM
 bool has_graphical_session() {
    const char* session = std::getenv("XDG_SESSION_TYPE");
    if (session) {
        if (std::strcmp(session, "wayland") == 0 || std::strcmp(session, "x11") == 0)
            return true;
        if (std::strcmp(session, "tty") == 0)
            return false;
    }

    if (std::getenv("WAYLAND_DISPLAY"))
        return true;

    if (std::getenv("DISPLAY"))
        return true;

    return false;
}
#endif // HAVE_GTKMM
