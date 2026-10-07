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
  * @brief Declare a file template.
  */

 #pragma once
 #include <udjat/defs.h>
 #include <udjat/tools/template.h>
 #include <reinstall/tools/datasource.h>
 #include <vector>

 namespace Reinstall {

	class UDJAT_API Template : public std::string, public Reinstall::DataSource::Item, protected Udjat::Template {
	public:
		
		/// @brief Build a template from properties.
		Template(const Udjat::Properties &props);

		/// @brief Build a template.
		Template(const char *name, const char *path);

		void load(const std::function<bool(uint64_t current, const void *buf, size_t length)> &writer) override;

		/// @brief Save item to file.
		/// @return The path to item file.
		std::string save() override;

		virtual void apply(std::ostream &stream) = 0;
		
	};

 }

//  #pragma once
//  #include <udjat/defs.h>
//  #include <udjat/tools/properties.h>
//  #include <udjat/tools/object.h>
//  #include <vector>

//  namespace Reinstall {

// 	class UDJAT_API Template : public Udjat::NamedObject {
// 	public:

// 		enum Type : uint8_t {
// 			Text		= 0x01,		///< @brief Template is a simple text file.
// 			Binary		= 0x04,		///< @brief Template is a binary file, just replace it.
// 		};

// 		Template(const Udjat::Properties &node);
// 		virtual ~Template();

// 		bool getProperty(const char *key, std::string &value) const override;

// 		static void load(const Udjat::Abstract::Object &parent, const Udjat::Properties &node, std::vector<std::shared_ptr<Template>> &templates);

// 		// Check if template match path.
// 		bool operator==(const char *path) const;

// 		/// @brief Parse template, save to file.
// 		/// @param parent The parent object, get values from it.
// 		/// @param path The target file path.
// 		/// @param current callback for progress dialog.
// 		void save(const Udjat::Abstract::Object &parent, const char *path, const std::function<bool(uint64_t current, uint64_t total)> &progress);

// 		/// @brief Parse template, save to defined path.
// 		/// @param parent The parent object, get values from it.
// 		/// @param current callback for progress dialog.
// 		inline void save(const Udjat::Abstract::Object &parent, const std::function<bool(uint64_t current, uint64_t total)> &progress) {
// 			save(parent,path,progress);
// 		}

// 	private:

// 		Type type = (Type) 0;
// 		bool escape = false;			///< @brief True if escape sequences should be processed.
// 		bool script = false;			///< @brief True if this is a script template.
// 		char marker = '$';
// 		mode_t mode = 0644;

// 		const char *url = nullptr;
// 		const char *path = nullptr;
// 		const char *quirk = nullptr;	///< @brief Quirk to apply to the template.

// 		std::string tempfilename;		///< @brief The temporary file with template data.

// 	};

//  }
