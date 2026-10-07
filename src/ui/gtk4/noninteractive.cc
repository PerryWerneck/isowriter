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
 #include <glib/gi18n.h>
 #include <private/toplevel.h>
 #include <udjat/ui/progress.h>
 #include <reinstall/group.h>
 #include <reinstall/action.h>
 #include <memory>
 #include <semaphore.h>
 #include <udjat/tools/threadpool.h>
 #include <reinstall/application.h>

 #ifdef LOG_DOMAIN
	#undef LOG_DOMAIN
 #endif
 #define LOG_DOMAIN "toplevel"
 #include <udjat/tools/logger.h>

 #pragma GCC diagnostic ignored "-Wattributes"
 #include <gtkmm.h>

 using namespace std;
 using namespace Udjat;

 NonInteractiveWindow::NonInteractiveWindow() : TopLevel() {
 
	// Setup the window
	set_deletable(false);
	set_resizable(false);
	set_default_size(700, -1);

	get_style_context()->add_class("dialog-progress");

	set_child(status);


	// Initialize, parse XML files.
	start();

 }

 NonInteractiveWindow::~NonInteractiveWindow() {
 
 }

 void NonInteractiveWindow::loaded() noexcept {
	Glib::signal_idle().connect_once([this](){
		activate();
	});
 }

 void NonInteractiveWindow::activate() noexcept {

	present();

	ThreadPool::getInstance().push([this](){
		
		IsoWriter::Application::activate();

		// Close progress popup.
		Glib::signal_idle().connect_once([](){
			gtk_window_close(gtk_application_get_active_window(GTK_APPLICATION(g_application_get_default())));
		});

	});

 }

 std::shared_ptr<IsoWriter::Group> NonInteractiveWindow::group_factory(const Udjat::Properties &node) {

	class Group : public IsoWriter::Group {
	private:
		NonInteractiveWindow &hwnd;

	public:

		Group(NonInteractiveWindow *h, const Udjat::Properties &node) : hwnd{*h} {
			// Initialize group with the XML node.
		}

		void push_back(const Udjat::Properties &node, shared_ptr<IsoWriter::Action> action) override {

			if(!IsoWriter::Action::is_default(node)) {
				return;
			}

			sem_t semaphore;
			sem_init(&semaphore,0,0);

			Glib::signal_idle().connect_once([this,&node,action,&semaphore](){
				auto &status = Udjat::Dialog::Status::getInstance();
				status.title(action->title());
				status.sub_title(_("Initializing..."));
				status.icon(action->icon());
				hwnd.select(action);
				sem_post(&semaphore);
			});

			sem_wait(&semaphore);

		}

	};

	return std::make_shared<Group>(this, node);

 }


 