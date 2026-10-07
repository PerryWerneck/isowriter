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
  * @brief Declare a group
  */

 #pragma once
 #include <udjat/defs.h>
 #include <udjat/tools/properties.h>
 #include <udjat/tools/string.h>
 #include <memory>

 namespace IsoWriter {

	class Action;

	class UDJAT_API Group : public Udjat::String {
	public:

		Group(const Udjat::Properties &props);
		virtual ~Group();

		/// @brief Parse xml node, build children.
		/// @param node The definitions for this group.
		virtual void parse(const Udjat::Properties &node);

		/// @brief Insert action on group.
		/// @param props The properties to setup group.
		/// @param action The action to insert.
		virtual void push_back(const Udjat::Properties &props,std::shared_ptr<Action> action) = 0;

		/// @brief Get label (for menu and logging);
		virtual const char *label() const noexcept = 0;

	};

 }

