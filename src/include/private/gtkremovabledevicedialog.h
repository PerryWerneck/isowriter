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
  * @brief Describe Gtk based dialog for detecting USB devices
  */

 #pragma once
 #include <config.h>
 #include <udjat/defs.h>
 #include <gtkmm.h>
 #include <reinstall/tools/writer.h>
 #include <reinstall/tools/removabledevicedialog.h>
 #include <reinstall/dialog.h>

 #define USE_MESSAGE_DIALOG 1
 #define USE_DROPDOWN 1

 #ifdef USE_MESSAGE_DIALOG
 class UDJAT_PRIVATE GtkRemovableDeviceDialog : public Gtk::MessageDialog, public IsoWriter::RemovableDeviceDialog {
 private:
 #else
 class GtkRemovableDeviceDialog : public Gtk::Window, public IsoWriter::RemovableDeviceDialog {
 private:
 #endif // USE_MESSAGE_DIALOG

 #ifdef USE_DROPDOWN
	class DeviceHolder : public Glib::Object, public IsoWriter::RemovableDeviceDialog::DeviceHolder {
	public:

		static Glib::RefPtr<DeviceHolder> create(Type type, const char *devname, const char *descr) {
			return Glib::make_refptr_for_instance<DeviceHolder>(new DeviceHolder(type, devname, descr));
		}

	private:

		DeviceHolder(Type type, const char *name, const char *descr) :
			IsoWriter::RemovableDeviceDialog::DeviceHolder{type, name, descr} {
		}

	};

	/// @brief Dropdown contents.
	Glib::RefPtr<Gio::ListStore<DeviceHolder>> store = Gio::ListStore<DeviceHolder>::create();
	Gtk::DropDown dropdown{store};

 #endif // USE_DROPDOWN

	/// Watch Storage devices.
	Glib::RefPtr<Gio::VolumeMonitor> volume_monitor;

	void setup(bool allow_output_to_file);

	/// @brief Select filename for output.
	void select_file();

	/// @brief Cancel button.
	Gtk::Button cancel;

	/// @brief Apply button.
	Gtk::Button apply;

	/// @brief Device was added.
	void append(const char *devname, const char *description) override;

	/// @brief Device was removed.
	void remove(const char *devname, const char *description) override;

	/// @brief Device was selected.
	void device_selected(Glib::RefPtr<DeviceHolder> device);

 public:
	GtkRemovableDeviceDialog(IsoWriter::Writer &writer, const IsoWriter::Dialog &dialog, bool allow_output_to_file = true);

	/// @brief Get selected device description.
	const char *description() const override;

	/// @brief Get selected device path
	const char *device() const override;

 };

