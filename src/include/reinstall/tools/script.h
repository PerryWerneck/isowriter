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
  * @brief Declare script tool.
  */

 #pragma once
 #include <udjat/defs.h>
 #include <udjat/tools/properties.h>
 #include <udjat/tools/object.h>
 #include <reinstall/tools/datasource.h>

 namespace IsoWriter {

	class UDJAT_API Script : private IsoWriter::FileSource {
	public:

		Script(const Udjat::Abstract::Object &parent, const Udjat::Properties &node);
		~Script();

		enum RunTime : uint8_t {
			Pre,
			Post
		};

		static void load(const Udjat::Abstract::Object &parent, const Udjat::Properties &node, std::vector<std::shared_ptr<Script>> &scripts);

		void run(const Udjat::Abstract::Object &object, const RunTime rtime, const char *msg = "");

	private:

		RunTime rtime = Post;
		char marker = '$';
		int uid = -1;
		int gid = -1;

		std::string tempfilename;

		const char *cmdline = nullptr;	///< @brief The command line to be executed
		const char *code = nullptr;		///< @brief The contents from xml, parsed and 'quarked'

	};

 }

