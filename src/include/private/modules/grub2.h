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
 #include <udjat/module.h>
 #include <udjat/tools/properties.h>
 #include <memory>
 #include <udjat/tools/properties.h>

 namespace IsoWriter {

	namespace Grub2 {

		class UDJAT_API Module : public Udjat::Module, public Udjat::Properties::Parser {
		private:
			class Action;

		public:
			static Udjat::Module * Factory(const char *name = "grub2");

			Module(const char *name = "grub2");
			virtual ~Module();

			bool parse(const Udjat::Properties &node) override;
	
		};
	}
 }

