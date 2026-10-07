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

 #pragma GCC diagnostic ignored "-Wattributes"
 #include <gtkmm.h>

 using namespace std;
 using namespace Udjat;

 class InteractiveWindow::Item : public Gtk::ToggleButton {
 private:

	class Label : public Gtk::Label {
	public:
		// https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1Label.html
		Label(const Udjat::Properties &node, const char *style, const char *attrname) 
		: Gtk::Label{node.get(attrname).c_str(), Gtk::Align::START} {
			get_style_context()->add_class(style);
		}
	
	} label, body;

	Gtk::Grid grid;
	Gtk::LinkButton help_button;

 public:
	Item(const Udjat::Properties &node, std::shared_ptr<IsoWriter::Action> action) 
		: label{node,"action-title","title"}, body{node,"action-subtitle","sub-title"} {

		set_hexpand(true);
		set_vexpand(false);
		set_valign(Gtk::Align::START);
		set_halign(Gtk::Align::FILL);

		label.set_text(node["title"]);
		body.set_text(node["sub-title"]);

		get_style_context()->add_class("action-button");
		grid.get_style_context()->add_class("action-container");

		auto icon_name = node["icon-name"];
		int margin = 0;
		if(!icon_name.empty()) {

			debug("Using icon '",icon_name.c_str(),"'");
			margin = 1;
	
			Gtk::Image image;
			image.set_icon_size(Gtk::IconSize::LARGE);
			image.get_style_context()->add_class("action-icon");
			image.set_from_icon_name(icon_name);
	
			grid.attach(image,0,0,1,2);
		}
	
		grid.attach(label,margin,0);
		grid.attach(body,margin,1);
	
		set_child(grid);
	
		get_style_context()->add_class("action-inactive");
		
		set_sensitive(false);

		// Check availability
		ThreadPool::getInstance().push([this,action](){

			try {
	
				if(action && !action->initialize()) {
					Logger::String{"Action initialization has returned 'false', keeping it disabled"}.error(action->name());
					return;
				}
	
			} catch(const std::exception &e) {
	
				Logger::String{e.what()}.error(action->name());
				return;
	
			} catch(...) {
	
				Logger::String{"Unexpected error while initializing action"}.error(action->name());
				return;
	
			}
	
			Logger::String{"Initialization complete, enabling item"}.info(action->name());
			Glib::signal_idle().connect([this](){
				set_sensitive(true);
				return 0;
			});
	
		});
	
	}
	
 };

 InteractiveWindow::InteractiveWindow() : TopLevel(), apply{"suggested-action",_("C_ontinue")}, cancel{"cancel-action",_("_Cancel")} {
 
	set_default_size(800, 600);

	// Set up layout
	set_child(hbox);
	hbox.append(sidebar);
	hbox.append(vbox);

	// Title
	{
		Gtk::Label title{_("Select an option")};
		title.get_style_context()->add_class("main-title");
		title.set_hexpand(true);
		title.set_vexpand(false);
		title.set_valign(Gtk::Align::START);
		vbox.append(title);
	}

	// Options
	{
		optionbox.set_margin_end(6);
		optionbox.set_hexpand(true);
		optionbox.set_vexpand(true);
		viewport.set_child(optionbox);
		viewport.set_hexpand(true);
		viewport.set_vexpand(true);
		vbox.append(viewport);
	}

	// Button bar
	{
		buttons.set_hexpand(false);
		buttons.set_vexpand(false);
		buttons.set_valign(Gtk::Align::END);
		buttons.set_halign(Gtk::Align::END);
		buttons.set_homogeneous();	
		buttons.get_style_context()->add_class("button-box");
		buttons.set_spacing(3);
		buttons.append(cancel);
		buttons.append(apply);
		set_default_widget(apply);
		apply.set_sensitive(false);
		vbox.append(buttons);

		cancel.signal_clicked().connect([&]() {
			Gtk::Window::close();
		});

		apply.signal_clicked().connect([&]() {
			activate();
		});
		
	}

	// Initialize, parse XML files.
	start();

 }

 InteractiveWindow::~InteractiveWindow() {
 
 }

 InteractiveWindow::SideBar::SideBar() : Gtk::Box{Gtk::Orientation::VERTICAL} {
	get_style_context()->add_class("toplevel-sidebar");
	set_hexpand(false);
	set_vexpand(true);
	set_homogeneous(false);

#ifdef DEBUG
	Config::Value<string> path{"defaults","sidebar-logo","./icons/logo.svg"};
#else
	Config::Value<string> path{"defaults","sidebar-logo",Udjat::Application::DataFile("icons/logo.svg").c_str()};
#endif // DEBUG

	try {

		Logger::String{"Getting logo from '",path.c_str(),"'"}.trace("sidebar");

		logo.set_pixel_size(128);
		logo.set(Gdk::Pixbuf::create_from_file(path.c_str()));
		logo.get_style_context()->add_class("toplevel-sidebar-logo");
		append(logo);

	} catch(const std::exception &e) {

		Logger::String{path.c_str(),": ",e.what()}.error("sidebar");

	}
	
 }

 // https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1Label.html
 TopLevel::Label::Label(const char *style, const char *text, Gtk::Align align) : Gtk::Label{text} {
	get_style_context()->add_class(style);
	set_wrap(true);
	set_halign(align);
	set_hexpand(true);
	set_vexpand(false);
 }

 TopLevel::Button::Button(const char *style, const char *text) : Gtk::Button{text} {
	// https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1Button.html
	get_style_context()->add_class(style);
	set_halign(Gtk::Align::END);
	get_style_context()->add_class("pill");
	set_hexpand(false);
	set_vexpand(false);
	set_use_underline(true);
  }

  std::shared_ptr<IsoWriter::Group> InteractiveWindow::group_factory(const Udjat::Properties &node) {
	
	static std::shared_ptr<Item> active_item;	///< @brief The selected item.
	
	class Group : public Gtk::Grid, public IsoWriter::Group {
	private:
		Label title{"group-title",""};						///< @brief The group title.
		Label sub_title{"group-subtitle",""};				///< @brief The group sub-title.
		Gtk::Box contents{Gtk::Orientation::VERTICAL};		///< @brief The box with the options.
		std::vector<std::shared_ptr<Item>> items;			///< @brief The list of items in this group.

	public:
		Group(const Udjat::Properties &node) {

			get_style_context()->add_class("group-title-box");

			set_hexpand(true);
			set_halign(Gtk::Align::FILL);
		
			set_vexpand(false);
			set_valign(Gtk::Align::START);
		
			contents.set_hexpand(true);
			contents.set_halign(Gtk::Align::FILL);
		
			contents.set_vexpand(false);
			contents.set_valign(Gtk::Align::START);
			contents.get_style_context()->add_class("item-box");
		
			get_style_context()->add_class("group-box");
		
			title.get_style_context()->add_class("group-title");
			sub_title.get_style_context()->add_class("group-subtitle");
		
			int margin = 0;
		
			if(node.contains("icon")) {
				auto icon = node.get("icon","image-missing");
				debug("Using icon '",icon.c_str(),"'");
				margin = 1;
		
				Gtk::Image image;
				//image.set_icon_size(Gtk::IconSize::LARGE);
				image.set_pixel_size(32);
				image.get_style_context()->add_class("group-icon");
				image.set_from_icon_name(icon.c_str());
		
				attach(image,0,0,1,2);
				contents.get_style_context()->add_class("item-box-no-icon");
		
			} else {
		
				contents.set_margin_start(35);
				contents.get_style_context()->add_class("item-box-icon");
		
			}
		
			attach(title,margin,0);
			attach(sub_title,margin,1);
			attach(contents,margin,2);

			title.set_text(node["title"]);
			sub_title.set_text(node["sub-title"]);

		}

		void push_back(const Udjat::Properties &node, std::shared_ptr<IsoWriter::Action> action) override {

			sem_t semaphore;
			sem_init(&semaphore,0,0);

			Glib::signal_idle().connect_once([this,&node,action,&semaphore](){

				auto item = make_shared<Item>(node, action);
				items.push_back(item);

				// Process activation.
				item->signal_toggled().connect([this,item,action]() {

					auto context = item->get_style_context();

					if(item->get_active()) {
						if(active_item) {
							active_item->set_active(false);
						}
						active_item = item;
						context->remove_class("action-inactive");
						context->add_class("action-active");
						Application::getInstance().select(action);
					} else {
						context->remove_class("action-active");
						context->add_class("action-inactive");
					}

				});

				// Show item.
				contents.append(*item);
				item->set_active(action->is_default(node));

				item->set_visible(true);
				this->set_visible(true);

				sem_post(&semaphore);

			});

			sem_wait(&semaphore);
		}

	};
	
	// Build the group in the main thread, wait for it.
	Group *group = nullptr;

	sem_t semaphore;
	sem_init(&semaphore,0,0);

	Glib::signal_idle().connect([this,&node,&group,&semaphore](){
		group = new Group(node);
		group->set_visible(false);
		optionbox.append(*group);
		sem_post(&semaphore);
		return 0;
	});

	sem_wait(&semaphore);
	return std::shared_ptr<Group>{group};
  }

  void InteractiveWindow::select(std::shared_ptr<IsoWriter::Action> action) {

	apply.set_sensitive(action.get() != nullptr);
	Application::select(action);

  } 

  void InteractiveWindow::activate() noexcept {

	if(action->confirmation && !action->confirmation->ask()) {
		Logger::String{"Action was cancelled"}.info(action->name());
		return;
	}


	/// @brief The GTK4 status dialog.
	class Status : public Gtk::Dialog {
	private:

		class StatusDialog : public TopLevel::Status {
		public:
			StatusDialog() : TopLevel::Status() {
			}

			~StatusDialog() override {
				debug("Destroying GTK4 status dialog");
			}

			Udjat::Dialog::Status & show() noexcept override {
				auto parent = gtk_widget_get_parent(GTK_WIDGET(gobj()));
				while(parent) {
					if(GTK_IS_WINDOW(parent)) {
						gtk_widget_set_visible(parent,TRUE);
						return *this;
					}
					parent = gtk_widget_get_parent(parent);
				}
				return *this;
			}

			Udjat::Dialog::Status & hide() noexcept override {
				auto parent = gtk_widget_get_parent(GTK_WIDGET(gobj()));
				while(parent) {
					if(GTK_IS_WINDOW(parent)) {
						gtk_widget_set_visible(parent,FALSE);
						return *this;
					}
					parent = gtk_widget_get_parent(parent);
				}
				return *this;
			}

		} status;

	public:
		Status() {

			gtk_window_set_transient_for(
				GTK_WINDOW(gobj()),
				gtk_application_get_active_window(GTK_APPLICATION(g_application_get_default()))
			);

			set_modal(true);
			set_deletable(false);
			set_resizable(false);

			set_decorated(false);
			set_default_size(500,-1);

			get_style_context()->add_class("dialog-progress");

			set_child(status);

			set_default_size(700, -1);

		}
		
		~Status() override {
			debug("Destroying GTK4 status dialog");
		}

	};

	// Activate the action.
	auto status = new Status();
	status->present();

	ThreadPool::getInstance().push([this,status](){
		
		IsoWriter::Application::activate();

		// Close progress popup.
		Glib::signal_idle().connect_once([status](){
			delete status;
		});

	});

  };
