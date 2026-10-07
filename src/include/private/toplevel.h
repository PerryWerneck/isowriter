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
  * @brief Declare application window.
  */

 #pragma once

 #include <config.h>
 #include <udjat/defs.h>
 #include <gtkmm.h>
 #include <memory>
 #include <string>
 #include <reinstall/application.h>
 #include <reinstall/dialog.h>
 #include <udjat/tools/properties.h>
 #include <reinstall/tools/writer.h>
 #include <reinstall/action.h>
 #include <udjat/ui/progress.h>
 #include <udjat/ui/status.h>

 class UDJAT_PRIVATE TopLevel : public Gtk::ApplicationWindow, protected IsoWriter::Application, private IsoWriter::Writer {
 private:
 public:
	TopLevel();
	~TopLevel() override;

 protected:

  	class Label : public Gtk::Label {
	public:
	  // https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1Label.html
	  Label(const char *style, const char *text, Gtk::Align align = Gtk::Align::START);
	
	};
	
	class Button : public Gtk::Button {
	public:
	  Button(const char *style, const char *text);

	};

	class Progress : public Gtk::Grid, public Udjat::Dialog::Progress {
	private:	
		Gtk::ProgressBar bar;
		Label left{"dialog-left-label","",Gtk::Align::START};
		Label right{"dialog-right-label","",Gtk::Align::END};

		Glib::RefPtr<Glib::TimeoutSource> timer;
		unsigned int idle = (unsigned int) -1;

		bool changed = false;
		uint64_t current = 0;
		uint64_t total = 0;

	public:
		Progress();
		~Progress() override;

		Udjat::Dialog::Progress & step(const unsigned int current = 0, const unsigned int total = 0) noexcept override;
			
		Udjat::Dialog::Progress & set(uint64_t current, uint64_t total, bool) noexcept override;

		Udjat::Dialog::Progress & url(const char *url) noexcept override;

	};
	
	class Status : public Gtk::Grid, private Udjat::Dialog::Progress::Factory, protected Udjat::Dialog::Status {
	private:
		Label main{"dialog-title",""}, subtitle{"dialog-subtitle",""};
		Gtk::Image side_icon;

		struct {
			std::string label;
			uint8_t changed = 0;
			bool valid = false;
			float fraction;
		} values;

		std::shared_ptr<Progress> progress;

	public:
		Status();
		~Status() override;

		std::shared_ptr<Udjat::Dialog::Progress> ProgressFactory() const;
		Udjat::Dialog::Status & title(const char *text) noexcept override;
		Udjat::Dialog::Status & sub_title(const char *text) noexcept override;
		Udjat::Dialog::Status & icon(const char *icon_name) noexcept override;
		Udjat::Dialog::Status & step(unsigned int current, unsigned int total) noexcept override;
		Udjat::Dialog::Status & state(const char *text) noexcept override;
		Udjat::Dialog::Status & busy(bool enable) noexcept override;
		Udjat::Dialog::Status & busy(const char *text) noexcept override;

	};

	/// @brief Start XML parsing
	void start();

	void failed(const std::exception &e) noexcept override;

	/// @brief Open target device for writing.
	/// @see IsoWriter::Writer::open()
	/// @return true if the device was open, false if not.
	bool open(const IsoWriter::Dialog &settings) override;

	std::shared_ptr<IsoWriter::Dialog> DialogFactory(const char *name, const Udjat::Properties &node, const char *message, const IsoWriter::Dialog::Option option) override;	

 };

 class UDJAT_PRIVATE NonInteractiveWindow : public TopLevel {
 private:
	TopLevel::Status status;

 public:
	NonInteractiveWindow();
	~NonInteractiveWindow() override;

	std::shared_ptr<IsoWriter::Group> group_factory(const Udjat::Properties &node) override;

	void activate() noexcept override;
	void loaded() noexcept override;
 };

 class UDJAT_PRIVATE InteractiveWindow : public TopLevel {
 private:

	class Item;

	class SideBar : public Gtk::Box {
	private:
		Gtk::Image logo;
   
	public:
	   SideBar();
   
	} sidebar;
   
	Button apply;
	Button cancel;

	Gtk::Box hbox{Gtk::Orientation::HORIZONTAL};
	Gtk::Box vbox{Gtk::Orientation::VERTICAL};

	Gtk::Box optionbox{Gtk::Orientation::VERTICAL};
	Gtk::Box buttons{Gtk::Orientation::HORIZONTAL};
	Gtk::ScrolledWindow viewport;

 public:
	InteractiveWindow();
	~InteractiveWindow() override;

 protected:
	std::shared_ptr<IsoWriter::Group> group_factory(const Udjat::Properties &node) override;

	void select(std::shared_ptr<IsoWriter::Action> action) override;

	void activate() noexcept override;

 };

 