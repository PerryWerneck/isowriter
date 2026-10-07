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
  * @brief Declare text mode interface.
  */

 #pragma once

 #include <config.h>
 #include <udjat/defs.h>
 #include <vector>
 
 #include <reinstall/application.h>
 #include <udjat/ui/status.h>
 #include <reinstall/tools/writer.h>

 namespace IsoWriter {

	class UDJAT_API Console : public IsoWriter::Application, private Udjat::Dialog::Status, private IsoWriter::Writer {
	private:
		class Group;

		std::vector<std::shared_ptr<Group>> groups;	///< @brief The list of groups.

	protected:

		/// @brief Build a new group.
		std::shared_ptr<IsoWriter::Group> group_factory(const Udjat::Properties &node) override;

		/// @brief Notify the user that the operation has failed.
		/// @param e The exception that was thrown.
		void failed(const std::exception &e) noexcept override;

	public:
		/// @brief Constructor.
		Console();

		/// @brief Destructor.
		virtual ~Console() noexcept;

		int run(int argc, char *argv[]);

		std::shared_ptr<IsoWriter::Dialog> DialogFactory(const char *name, const Udjat::Properties &node, const char *message, const Dialog::Option option = Dialog::None) override;	

		bool open(const IsoWriter::Dialog &settings) override;

	};
 }

