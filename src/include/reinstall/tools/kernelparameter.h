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
  * @brief Declare kernel parameter.
  */

 #pragma once

 #include <udjat/defs.h>
 #include <udjat/tools/properties.h>
 #include <udjat/tools/string.h>
 #include <memory>
 #include <vector>

 namespace IsoWriter {

	class UDJAT_API KernelParameter {
	private:

		/// @brief Command-line preset.
		struct Preset {
			const char *name;
			const char *value;
			Preset(const char *name, const char *value);
		};

		const char *object_name;
		static std::vector<Preset> presets;

	public:

		constexpr KernelParameter(const char *n) : object_name{n} {
		}

		KernelParameter(const Udjat::Properties &node) : object_name{Udjat::String{node,"name"}.as_quark()} {
		}

		virtual ~KernelParameter();

		// @brief Convenience method to expand values when building object.
		static const char * expand(const Udjat::Properties &node, const char *attrname);

		/// @brief Override xml defined kernel parameters.
		static inline void preset(const char *name, const char *value) {
			presets.emplace_back(name,value);
		}

		static void preset(const char *arg);

		/// @brief Get Parameter name.
		inline const char *parameter_name() const noexcept {
			return object_name;
		}

		/// @brief Convenience method to apply properties.
		static std::string value(const Udjat::Abstract::Object &object, const char *str);

		/// @brief Apply properties
		/// @return The parameter value with object properties applied.
		virtual std::string value(const Udjat::Abstract::Object &object) const = 0;

		/// @brief Load kernel parameters.
		/// @param node XML node to load from.
		/// @param parameters Array to store loaded parameters.
		/// @param relpaths If true set path to be relative to partition.
		static void load(const Udjat::Properties &node, std::vector<std::shared_ptr<KernelParameter>> &parameters, bool relpaths = false);
		static std::string join(const Udjat::Abstract::Object &object, const std::vector<std::shared_ptr<KernelParameter>> &parameters);

	};

 }
