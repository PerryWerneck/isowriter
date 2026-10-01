/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2026 Perry Werneck <perry.werneck@gmail.com>
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
  * @brief Declare main application.
  */

 #pragma once
 #include <udjat/defs.h>
 #include <udjat/tools/properties.h>
 #include <memory>
 #include <vector>
 #include <udjat/tools/string.h>
 #include <udjat/ui/progress.h>

 namespace Reinstall {

	class Group;
	class Action;
	class Progress;

	class UDJAT_PRIVATE Application : private Udjat::Properties::ObjectBuilder {
	private:
		static Application *instance;						///< @brief Singleton instance.
		static bool non_interactive_mode;					///< @brief If true set non-interactive mode, if false set interactive mode.	
		static std::vector<Udjat::String> selected_path;	///< @brief Selected path for auto-selecting image to build.

		std::shared_ptr<Group> find_group(const Udjat::Properties &props);
		
	protected:

		std::vector<std::shared_ptr<Group>> groups;		///< @brief List of groups.
		std::shared_ptr<Group> selected_group;			///< @brief Selected group.

		/// @brief Insert group, if already exists log warning and ignore.
		/// @param props The properties to setup group.
		/// @param group The group to insert.
		/// @return true if the group was selected.
		bool push_back(const Udjat::Properties &props,std::shared_ptr<Group> group);

		/// @brief Insert action on group.
		/// @param props The properties to get the target group.
		/// @param action The action to insert.
		/// @return true if the action was selected.
		virtual bool push_back(const Udjat::Properties &props,std::shared_ptr<Action> action);

		/// @brief Run interactive mode.
		/// @return return code (0 = ok)
		virtual int run_interactive();

		/// @brief Run interactive mode.
		/// @return return code (0 = ok)
		virtual int run_non_interactive();

	public:

		Application();
		virtual ~Application();

		/// @brief Load configuration files, call user interaction.
		virtual int run();

		static int run_tui();

		static Application & get_instance();

		static void set_selected_path(const char *path);

		/// @brief Build the image.
		/// @param props The properties for the build.
		/// @return true if the build was successful.
		bool build(const Udjat::Properties &props) override;

		virtual std::shared_ptr<Reinstall::Progress> ProgressDialogFactory();

		/// @brief Set non-interactive mode, usually for scripts.
		/// @param value If true set non-interactive mode, if false set interactive mode.
		static inline void non_interactive(bool value) noexcept {
			non_interactive_mode = value;
		}

		static inline bool non_interactive() noexcept {
			return non_interactive_mode;
		}

	};

 }

