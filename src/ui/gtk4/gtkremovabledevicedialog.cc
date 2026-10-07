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
  * @brief Implements gtk device selection dialog.
  */

 // https://fossies.org/linux/gtkmm/demos/gtk-demo/example_dropdown.cc
 // http://storaged.org/doc/udisks2-api/latest/ref-dbus.html
 // https://stackoverflow.com/questions/63537158/how-to-list-all-the-removable-devices-with-dbus-and-udisks2
 // https://github.com/GNOME/glibmm/blob/master/examples/dbus/client_bus_listnames.cc
 // http://transit.iut2.upmf-grenoble.fr/doc/glibmm-2.4/reference/html/classGio_1_1DBus_1_1Proxy.html

 #include <config.h>
 #include <udjat/defs.h>

 #include <private/gtkremovabledevicedialog.h>
 #include <gtkmm.h>
 #include <glib/gi18n.h>

 #if GTK_CHECK_VERSION(4,10,0)
 #include <gtkmm/filedialog.h>
 #endif

 #include <udjat/tools/intl.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/mainloop.h>
 #include <udjat/tools/threadpool.h>

 #include <reinstall/dialog.h>

 using namespace Udjat;
 using namespace std;

#ifdef USE_MESSAGE_DIALOG

 GtkRemovableDeviceDialog::GtkRemovableDeviceDialog(IsoWriter::Writer &w, const IsoWriter::Dialog &dialog, bool allow_output_to_file)
 : 	Gtk::MessageDialog{"",false,Gtk::MessageType::QUESTION,Gtk::ButtonsType::NONE}, 
 	volume_monitor{Gio::VolumeMonitor::get()}, 	cancel{_("_Cancel"),true},
	apply{_("C_ontinue"),true} {

 	add_action_widget(cancel,ECANCELED);
	add_action_widget(apply,0);
	apply.set_sensitive(false);
	apply.get_style_context()->add_class("destructive-action");

	set_message(dialog.text(_("Insert an storage device")),true	);
	set_secondary_text(dialog.body(_("This action will <b>DELETE ALL CONTENT</b> on the device.")),true);

	setup(allow_output_to_file);

	volume_monitor->signal_drive_connected().connect([&](const Glib::RefPtr<Gio::Drive> drive){
		if(drive->is_removable()) {
			append(drive->get_identifier(G_DRIVE_IDENTIFIER_KIND_UNIX_DEVICE).c_str(),drive->get_name().c_str());
		}
	});

	volume_monitor->signal_drive_disconnected().connect([&](const Glib::RefPtr<Gio::Drive> drive){
		if(drive->is_removable()) {
			remove(drive->get_identifier(G_DRIVE_IDENTIFIER_KIND_UNIX_DEVICE).c_str(),drive->get_name().c_str());
		}
	});

#ifdef USE_DROPDOWN
	dropdown.get_style_context()->add_class("device-selector");
	get_message_area()->append(dropdown);
#endif // USE_DROPDOWN

 }
#else
 GtkRemovableDeviceDialog::GtkRemovableDeviceDialog() {
	setup(allow_output_to_file);

#				Gtk::Box view{Gtk::Orientation::VERTICAL};
	view.set_hexpand(true);
	view.set_vexpand(true);

	Gtk::Box contents{Gtk::Orientation::VERTICAL};
	contents.set_hexpand(true);
	contents.set_vexpand(true);
	contents.set_spacing(6);
	contents.set_margin(12);

	{
		Gtk::Label label;
		label.set_markup(
			Config::Value<string>{
				"defaults",
				"insert-device-message",
				_("Insert an storage device <b>NOW</b>")
			}.c_str()
		);
		label.get_style_context()->add_class("dialog-title");
		label.set_vexpand(false);
		contents.append(label);
	}

	{
		Gtk::Label label;
		label.set_markup(
			Config::Value<string>{
				"defaults",
				"insert-device-body",
				_("This action will <b>DELETE ALL CONTENT</b> on the device.")
			}.c_str()
		);
		label.get_style_context()->add_class("dialog-subtitle");
		label.set_vexpand(false);
		contents.append(label);
	}

	view.append(contents);

	{
		Gtk::Box action_box{Gtk::Orientation::HORIZONTAL};
		action_box.get_style_context()->add_class("dialog-action-box");

		Gtk::Box buttons{Gtk::Orientation::HORIZONTAL};
		buttons.get_style_context()->add_class("dialog-action-area");
		buttons.set_hexpand(true);
		buttons.set_vexpand(false);
		buttons.set_homogeneous(true);

		cancel.set_hexpand(true);
		cancel.get_style_context()->add_class("text-button");
		cancel.set_use_underline(true);

		apply.set_hexpand(true);
		apply.get_style_context()->add_class("text-button");
		apply.set_use_underline(true);

		buttons.append(cancel);
		buttons.append(apply);

		action_box.append(buttons);
		view.append(action_box);
	}

	set_child(view);
}
#endif // USE_MESSAGE_DIALOG

 void GtkRemovableDeviceDialog::setup(bool allow_output_to_file) {

	gtk_window_set_transient_for(
		GTK_WINDOW(gobj()),
		gtk_application_get_active_window(GTK_APPLICATION(g_application_get_default()))
	);

	set_modal(true);
	set_default_size(600,-1);

#ifdef USE_DROPDOWN
	auto expression = Gtk::ClosureExpression<Glib::ustring>::create(
		[](const Glib::RefPtr<Glib::ObjectBase>& item)->Glib::ustring
		{
			return std::dynamic_pointer_cast<DeviceHolder>(item)->description;
		});

	dropdown.set_expression(expression);
	dropdown.set_enable_search();

	store->append(DeviceHolder::create(DeviceHolder::AutoDetect,"",_("Auto detect")));

#ifdef DEBUG
	store->append(DeviceHolder::create(DeviceHolder::File,"/tmp/test.iso","Test image at /tmp/test.iso"));
#endif // DEBUG

	dropdown.property_selected().signal_changed().connect([&](){

		debug("Device selection has changed");

		if (auto device = std::dynamic_pointer_cast<DeviceHolder>(dropdown.get_selected_item())) {

			debug("Selected device='",device->description.c_str(),"'");
			device_selected(device);

		} else {

			g_warning("Unexpected item in DropDown widget");

		}


	});

#if GTK_CHECK_VERSION(4,10,0)
	if(allow_output_to_file) {
		store->append(DeviceHolder::create(DeviceHolder::FileDialog,"",_("Save image to file")));
	}
#endif // GTK_CHECK_VERSION(4,10,0)

	// Load devices
	for(const auto &drive : volume_monitor->get_connected_drives()) {
		if(drive->is_removable()) {
			append(drive->get_identifier(G_DRIVE_IDENTIFIER_KIND_UNIX_DEVICE).c_str(),drive->get_name().c_str());
		}
	}

#endif // USE_DROPDOWN

 }

 void GtkRemovableDeviceDialog::select_file() {
 #if GTK_CHECK_VERSION(4,10,0)

	debug("Selecting filename");

	auto dialog = Gtk::FileDialog::create();

	dialog->set_title(_("Select output file"));
	dialog->set_modal();

	dialog->save(*this,[this,dialog](Glib::RefPtr<Gio::AsyncResult> result){

		try {

			auto file = dialog->save_finish(result)->get_path();
			store->append(DeviceHolder::create(DeviceHolder::File,file.c_str(),file.c_str()));
			dropdown.set_selected((dropdown.get_model()->get_n_items()-1));

		} catch(const std::exception &e) {

			Logger::String{e.what()}.error("FileDialog");
			dropdown.set_selected(0);

		}

	});
 #endif // GTK_CHECK_VERSION(4,10,0)

 }

 const char * GtkRemovableDeviceDialog::description() const {

#ifdef USE_DROPDOWN
	if (auto device = std::dynamic_pointer_cast<const DeviceHolder>(dropdown.get_selected_item())) {
		return device->description.c_str();
	}
#endif // USE_DROPDOWN

	return "";
 }

 const char * GtkRemovableDeviceDialog::device() const {

#ifdef USE_DROPDOWN
	if (auto device = std::dynamic_pointer_cast<const DeviceHolder>(dropdown.get_selected_item())) {
		return device->device_name.c_str();
	}
#endif // USE_DROPDOWN

	return "";
 }

 void GtkRemovableDeviceDialog::append(const char *devname, const char *description) {

	Logger::String("Device '",description,"' was inserted (",devname,")").info("dialog");

#ifdef USE_DROPDOWN
	store->append(DeviceHolder::create(DeviceHolder::Device,devname,description));
	auto device = std::dynamic_pointer_cast<const DeviceHolder>(dropdown.get_selected_item());
	if(device && device->type == DeviceHolder::AutoDetect) {
		debug("Auto detect enabled select the new device");
		dropdown.set_selected(store->get_n_items()-1);
	}
#endif // USE_DROPDOWN

 }

 void GtkRemovableDeviceDialog::remove(const char *devname, const char *description) {

 	Logger::String("Device '",description,"' was removed (",devname,")").info("dialog");

#ifdef USE_DROPDOWN
	auto device = std::dynamic_pointer_cast<const DeviceHolder>(dropdown.get_selected_item());
	if(device && device->type == DeviceHolder::Device && !strcmp(device->device_name.c_str(),devname)) {
		debug("Selected device was removed, selecting auto-detect");
		auto item = dropdown.get_selected();
		dropdown.set_selected(0);
		apply.set_label("C_ontinue");
		apply.set_sensitive(false);
		store->remove(item);
		return;
	}

	for(guint item = 0; item < store->get_n_items(); item++) {
		auto device = std::dynamic_pointer_cast<const DeviceHolder>(store->get_object(item));
		if(device && device->type == DeviceHolder::Device && !strcmp(device->device_name.c_str(),devname)) {
			store->remove(item);
			return;
		}
	}
#endif // USE_DROPDOWN

 }

 void GtkRemovableDeviceDialog::device_selected(Glib::RefPtr<DeviceHolder> device) {

	debug("Selected device: ",device->description.c_str());

 	IsoWriter::Writer::getInstance().close();

	switch(device->type) {
	case DeviceHolder::FileDialog:
		apply.set_label(_("C_ontinue"));
		select_file();
		break;

	case DeviceHolder::File:
 		apply.set_label(_("_Save image"));
		apply.set_sensitive(true);
		break;

	case DeviceHolder::Device:
		{
			string devname{device->device_name};
			debug("Checking ",devname.c_str());

			apply.set_sensitive(false);
			apply.set_label(_("Detecting..."));

			// Open device in a different thread
			// This is necessary because the device may be slow to open.
			// The UI can not be blocked while opening the device.
			ThreadPool::getInstance().push("devopen",[this,devname](){

				string error_message;

				try {

					debug("Opening device");
					IsoWriter::Writer::getInstance().open(devname.c_str());

				} catch(const std::system_error &e) {

					int err = e.code().value();

					debug("System error ",err);

					Logger::String{devname.c_str(),": ",e.what()," (",err,")"}.warning("dialog");

					switch(err) {
					case ENOSPC:
						error_message.assign(_("Not enough space"));
						break;

					case EPERM:
					case EACCES:
						error_message.assign(_("Access Denied"));
						break;

					default:
						error_message.assign(strerror(err));
					}

				} catch(const std::exception &e) {

					debug("Exception");

					Logger::String{devname.c_str(),": ",e.what()}.warning("dialog");
					error_message.assign(_("Invalid device"));

				}

				// Update UI in the main thread
				// This is necessary because the callback is executed in a different thread.
				// The UI can only be updated in the main thread.
				MainLoop::getInstance().run([this,error_message](){
					if(error_message.empty()) {
						apply.set_label(_("C_ontinue"));
						apply.set_sensitive((bool) IsoWriter::Writer::getInstance());
					} else {
						apply.set_label(error_message);
						apply.set_sensitive(false);
					}
				}); // MainLoop::run

			});

		}
		break;

	default:
		apply.set_label(_("C_ontinue"));
		apply.set_sensitive(false);
	}


 }
