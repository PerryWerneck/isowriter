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
  * @brief Declares a reinstall data source.
  */

 #pragma once
 #include <config.h>
 #include <udjat/defs.h>
 #include <reinstall/dialog.h>

 namespace IsoWriter {

	/// @brief Abstract class for removable device dialog.
	/// @details This class is used to detect removable devices, such as USB drives, and
	/// to notify the user when a device is added or removed.
	/// @note This class is not intended to be instantiated directly. Instead, you should
	/// derive from it and implement the `device_added` and `device_removed` methods.
	class UDJAT_API RemovableDeviceDialog {
	public:
		RemovableDeviceDialog();
		virtual ~RemovableDeviceDialog() = default;

	protected:

		struct DeviceHolder {

			enum Type : uint8_t {
				Undefined,
				AutoDetect,	///<< @brief Waiting for device.
				FileDialog,	///<< @brief File dialog.
				File,		///<< @brief File name.
				Device		///<< @brief Standard device entry.
			} type = Undefined;

			std::string	description;
			std::string	device_name;

			DeviceHolder(Type t, const char *name, const char *descr)
				: type{t}, description{descr}, device_name{name} {
			}

		};

		/// @brief Device was added.
		virtual void append(const char *devname, const char *description) = 0;

		/// @brief Device was removed.
		virtual void remove(const char *devname, const char *description) = 0;

		/// @brief Get selected device description.
		virtual const char *description() const = 0;

		/// @brief Get selected device path
		virtual const char *device() const = 0;

	};

 }

