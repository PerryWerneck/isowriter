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
  * @brief Declare internal SLP client.
  */

 #pragma once
 #include <udjat/defs.h>
 #include <udjat/tools/properties.h>
 #include <udjat/tools/object.h>

 namespace IsoWriter {

	class UDJAT_API SLPClient : public Udjat::NamedObject {
	private:
		const char *service_type = "";
		const char *scope_list = "";
		const char *filter = "";
		const char *message = "";	///< @brief The message for search dialog progress.
		bool allow_local = false;

		struct {
			std::string url;
			bool done = false;
		} query;

	public:
		SLPClient(const Udjat::Properties &node);

		bool operator==(const SLPClient &b) const noexcept;

		void clear() noexcept;

		/// @brief Detect url.
		/// @return URL from slp service, empty if not found.
		const char *url();

		inline const char * service() const noexcept {
			return service_type;
		}

		static std::shared_ptr<SLPClient> Factory(const Udjat::Properties &node);

	};

 }
