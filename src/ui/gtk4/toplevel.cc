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
  * @brief Implements gtk4 application window.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <glib/gi18n.h>
 #include <private/toplevel.h>
 #include <semaphore.h>
 #include <udjat/ui/progress.h>
 #include <udjat/ui/status.h>

 #ifdef LOG_DOMAIN
	#undef LOG_DOMAIN
 #endif
 #define LOG_DOMAIN "toplevel"
 #include <udjat/tools/logger.h>

 #include <udjat/tools/configuration.h>
 #include <udjat/tools/threadpool.h>
 #include <udjat/tools/mainloop.h>
 #include <udjat/tools/application.h>
 #include <string>
 #include <reinstall/group.h>
 #include <reinstall/action.h>
 #include <reinstall/tools/writer.h>

 #pragma GCC diagnostic ignored "-Wattributes"
 #include <gtkmm.h>
 #include <private/toplevel.h>
 
 using namespace std;
 using namespace Udjat;

 TopLevel::TopLevel() : Gtk::ApplicationWindow(), IsoWriter::Writer{"toplevel"} {

	// Get rid of the gtk warnings.
	if(!freopen("/dev/null","w",stderr)) {
		Logger::String{"Failed to redirect stderr to /dev/null: ",strerror(errno)}.warning();
	}

#ifdef DEBUG 
	get_style_context()->add_class("devel");
#endif

	// https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1ApplicationWindow.html
	{
		auto css = Gtk::CssProvider::create();
#ifdef DEBUG
		css->load_from_path("./stylesheet.css");
#else
		css->load_from_path(Udjat::Application::DataFile("stylesheet.css").c_str());
#endif // DEBUG
		get_style_context()->add_provider_for_display(Gdk::Display::get_default(),css,GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	}

	{
#ifdef DEBUG
		std::string iconpath{"./icons"};
#else
		Udjat::Application::DataDir iconpath{"icons"};
#endif // DEBUG

		Logger::String{"Searching '",iconpath.c_str(),"' for customized icons"}.trace();

		// https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1IconTheme.html
		Gtk::IconTheme::get_for_display(Gdk::Display::get_default())->add_search_path(iconpath);

	}

	set_deletable(false);

	// Set up layout
	set_title(Config::Value<string>("toplevel","title",_("System reinstallation")));

 }

 TopLevel::~TopLevel() {
 }

 void TopLevel::start() {

	set_sensitive(false);
	ThreadPool::getInstance().push([this](){

		try {

			load_options();
			Glib::signal_idle().connect([this](){
				set_sensitive(true);
				present();
				return 0;
			});
	
		} catch(const std::exception &e) {
			failed(e);
		}

	});
 }

 void TopLevel::failed(const std::exception &e) noexcept {

	Logger::String err{e.what()};
	
	err.error();

	Glib::signal_idle().connect([this,&err](){

		present();

		// TODO: Show error popup.
		debug("Showing error popup");
		
		this->Gtk::ApplicationWindow::close();

		return 0;
	});

 }

