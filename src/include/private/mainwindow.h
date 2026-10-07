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
  * @brief Declare application window.
  */

 #error deprecated
 #pragma once

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/version.h>
 #include <glib/gi18n.h>
 #include <gtkmm.h>

 #include <widgets/sidebar.h>

 #include <udjat/tools/factory.h>
 #include <udjat/tools/properties.h>

 #include <reinstall/group.h>
 #include <reinstall/action.h>
 #include <reinstall/tools/writer.h>

 #include <list>

 #if ! UDJAT_CHECK_VERSION(1,2,1)
 namespace Udjat {
	namespace XML {
		UDJAT_PRIVATE bool test(const XML::Node &node, const char *attrname, bool defvalue = true);
	}
 }
 #endif // UDJAT 1.2.1

 class UDJAT_PRIVATE MainWindow : public Gtk::ApplicationWindow, private IsoWriter::Group::Controller, private IsoWriter::GtkWriter {
 private:
	struct Layout {
		SideBar sidebar;
		Gtk::Box box;
		Gtk::Box vbox;
		Gtk::Label title;
		Gtk::Box contents;	///< @brief The box with the groups & options.
		Gtk::ScrolledWindow swindow;
		Layout();
	} layout;

	struct Button {
		Gtk::Button apply;
		Gtk::Button cancel;
		Gtk::Box box;
		Button();
	} button;

	class Item : public Gtk::ToggleButton {
	private:
		std::shared_ptr<Udjat::Abstract::Object> action;
		Gtk::Grid grid;
		Gtk::LinkButton help_button;
		Gtk::Label label, body;

	public:
		Item(const Udjat::Properties &node, std::shared_ptr<Udjat::Abstract::Object> action);
		virtual ~Item();

		inline operator bool() const noexcept {
			return (bool) action.get();
		}

		inline auto message() const noexcept {
			return label.get_text();
		}

		void activate() const;

	};

	Item *selected = nullptr;
	std::list<std::shared_ptr<Item>> itens;

	class Group : public Gtk::Grid, public IsoWriter::Group {
	private:
		Gtk::Label title;
		Gtk::Label sub_title;
		Gtk::Box contents{Gtk::Orientation::VERTICAL};	///< @brief The box with the options.

	public:
		Group(const Udjat::Properties &node);

		// Udjat::Abstract::Object
		void push_back(const Udjat::Properties &node, std::shared_ptr<Udjat::Abstract::Object> child) override;

	};

	static MainWindow *instance;

	static MainWindow & getInstance() {
		return *instance;
	}

 public:

	MainWindow(Glib::RefPtr<::Gtk::Application> app);
	virtual ~MainWindow();

	std::shared_ptr<IsoWriter::Group> get(const Udjat::Properties &node) override;

 };






