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
  * @brief Declares abstract disk writer.
  */

 #pragma once
 #include <udjat/defs.h>
 #include <string>
 #include <memory>

 namespace Reinstall {

	class UDJAT_API Writer : public std::string {
	protected:
		int fd = -1;

		/// @brief The pre-defined device name.
		static std::string devname;

	public:

		static std::shared_ptr<Writer> get_instance(unsigned long long length = 0);

		/// @brief Build a writer for the device.
		/// @param device_name The destination device.
		Writer(const char *device_name, unsigned long long length = 0);

		Writer(int fd, unsigned long long length = 0);

		virtual ~Writer();

 		/// @brief Set device output.
 		/// @param path The device path.
 		static void set_device_name(const char *path);

		/// @brief Write data to device.
		/// @param length Block length.
		void write(unsigned long long offset, const char *contents, unsigned long long length);

	};

 }

//  #include <udjat/defs.h>
//  #include <reinstall/dialog.h>
//  #include <string>
//  #include <udjat/tools/string.h>
//  #include <udjat/tools/logger.h>

//  namespace Reinstall {

// 	/// @brief Abstract disk device.
// 	class UDJAT_API Writer {
// 	private:

// 		/// @brief The writer name.
// 		const char *writer_name = nullptr;
		
// 		/// @brief The active writer instance.
// 		static Writer *instance;

// 		int fd = -1;
// 		unsigned long long length = 0LL;

// 	protected:
// 		Writer(const char *name);

// 		/// @brief The URL for progress.
// 		Udjat::String device_url;

// 		/// @brief Device set from command-line option.
// 		static std::string selected;

// 		/// @brief Allocate required space, exception if not enough.
// 		void allocate();

// 	public:
// 		virtual ~Writer();

// 		inline const char *name() const noexcept {
// 			return writer_name;
// 		}

// 		inline operator bool() const noexcept {
// 			return (bool) (fd != -1);
// 		}

// 		/// @brief Open device.
// 		void open(const char *device_name);

// 		/// @brief Close device.
// 		void close();

// 		/// @brief The device URL (for progress bar).
// 		inline const char *url() const noexcept {
// 			return device_url.c_str();
// 		}

// 		/// @brief Set output from comand-line
// 		static void set_output(const char *path);

// 		/// @brief Get writer instance.
// 		static Writer & getInstance();

// 		/// @brief Select/detect and open device.
// 		/// @return true if the device is open, false it not.
// 		virtual bool open(const Reinstall::Dialog &settings);

// 		/// @brief Get device length.
// 		/// @return The device length.
// 		/// @retval 0 The device length is undefined.
// 		unsigned long long size() const;

// 		/// @brief Set required device length.
// 		inline void size(unsigned long long length) {
// 			this->length = length;
// 		}

// 		/// @brief Write file to device
// 		void write(int fd);

// 		/// @brief Write iso image to device.
// 		void write(const char *isoname);

// 		/// @brief Write data to device.
// 		/// @param offset Offset of current block.
// 		/// @param length Block length.
// 		void write(unsigned long long offset, const void *contents, unsigned long long length);

// 	};

//  }

