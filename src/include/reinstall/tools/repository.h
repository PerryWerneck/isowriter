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
  * @brief Declare a repository.
  */

 #pragma once
 #include <udjat/defs.h>
 #include <udjat/tools/url.h>
 #include <udjat/tools/properties.h>
 #include <reinstall/tools/datasource.h>
 #include <functional>
 #include <vector>

 namespace Reinstall {

	class UDJAT_API Repository {
	private:
		void sanitize(const Udjat::URL &url);

	protected:
		bool allow_cache;
		DataSource::Item source;
		static std::string hostname;	///< @brief The fixed hostname.

	public:
	
		/// @brief Build datasource.
		/// @param remote URL for remote files.
		/// @param local URL for local files.
		Repository(const char *remote = nullptr, const char *local = nullptr);

		/// @brief Build URL using properties.
		/// @param props The properties for URL.
		Repository(const Udjat::Properties &props);

		/// @brief Set URL for installation repository.
		/// @param install The URL to set
		static void url(const char *install);

		/// @brief Set hostname for all repositories.
		static void host(const char *hostname);

		/// @brief Load repository index.
		/// @param itens Vector to receive the repository contents.
		void load(std::vector<DataSource::Item> itens);

		virtual ~Repository();

	};

 }

//  #include <udjat/defs.h>
//  #include <udjat/tools/properties.h>
//  #include <udjat/tools/object.h>
//  #include <memory>
//  #include <vector>

//  #include <reinstall/tools/datasource.h>
//  #include <reinstall/tools/kernelparameter.h>

//  namespace Reinstall {

// 	class SLPClient;

// 	class UDJAT_API Repository : public FileSource, public KernelParameter {
// 	private:

// 		/// @brief Command-line preset.
// 		struct Preset {
// 			const char *name;
// 			const char *value;
// 			Preset(const char *name, const char *value);
// 		};

// 		static std::vector<Preset> presets;

// 		struct KParm {
// 			bool enabled = true;
// 			const char *name = nullptr;
// 			const char *slp = nullptr;

// 			KParm(const Udjat::Properties &node);

// 		} kparm;

// 		std::shared_ptr<SLPClient> slpclient;

// 		/// @brief The repository files (from INDEX.gz)
// 		std::vector<std::string> files;

// 		/// @brief Load index from filename.
// 		bool index(const char *filename);

// 	public:

// 		static std::shared_ptr<Repository> Factory(const Udjat::Properties &node);

// 		Repository(const Udjat::Properties &node);
// 		virtual ~Repository();

// 		bool operator==(const Repository &repo) const noexcept;

// 		inline bool is_kernel_parameter() const noexcept {
// 			return kparm.name && *kparm.name;
// 		}

// 		/// @brief Override xml defined remote URL.
// 		static inline void preset(const char *name, const char *value) {
// 			presets.emplace_back(name,value);
// 		}

// 		static void preset(const char *arg);

// 		// KernelParameter
// 		std::string value(const Udjat::Abstract::Object &object) const override;

// 		/// @brief Load repository index (INDEX.gz)
// 		/// @return true if the repository has an index.
// 		bool index();

// 		const char * remote() const override;

// #if __cplusplus >= 201703L

// 		inline const auto begin() const noexcept {
// 			return files.begin();
// 		}

// 		inline const auto end() const noexcept {
// 			return files.end();
// 		}

// #else

// 		inline std::vector<std::string>::const_iterator begin() const noexcept {
// 			return files.begin();
// 		}

// 		inline std::vector<std::string>::const_iterator end() const noexcept {
// 			return files.end();
// 		}

// #endif

// 	};

//  }


