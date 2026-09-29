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

 #pragma once
 
 #include <udjat/defs.h>
 #include <udjat/tools/properties.h>
 #include <udjat/tools/url.h>

 namespace Reinstall {

	class Repository;

	class UDJAT_API DataSource {
	protected:
		std::shared_ptr<Repository> repo;	///< @brief Repository for relative URLs.
		Udjat::URL local;					///< @brief URL for local file.
		Udjat::URL remote;					///< @brief URL for remote file.
		bool allow_cache;

	public:

		/// @brief Build datasource.
		/// @param repository The repository.
		/// @param path Relative path for files inside repository.
		DataSource(std::shared_ptr<Repository> repository, const char *path);
	
		/// @brief Build datasource.
		/// @param remote URL for remote files.
		/// @param local URL for local files.
		DataSource(const char *remote = nullptr, const char *local = nullptr);

		/// @brief Build URL using properties.
		/// @param props The properties for URL.
		DataSource(const Udjat::Properties &props);

		virtual ~DataSource();

		/// @brief Is this datasource a directory?
		bool dir() const noexcept;

		/// @brief Get URL for datasource file.
		Udjat::URL url();

	};

 }

//  #include <udjat/tools/properties.h>
//  #include <udjat/tools/object.h>
//  #include <udjat/tools/url.h>
//  #include <udjat/tools/string.h>
//  #include <udjat/ui/progress.h>
//  #include <memory>
//  #include <vector>

//  namespace Reinstall {

// 	class Repository;
// 	class Template;

// 	class UDJAT_API DataSource : public Udjat::NamedObject {
// 	protected:

// 		const char *message = "";
// 		bool allow_cache = true;

// 		std::shared_ptr<Repository> repository;

// 		/// @brief When true allways check file timestamp with remote server.
// 		bool update_from_remote = true;

// 		// const char * PathFactory(const Udjat::Properties &node, const char *attrname, bool required = true) const;

// 		DataSource() {
// 		}

// 		DataSource(const DataSource &src);

// 	public:

// 		DataSource(const char *name) : Udjat::NamedObject(name) {}

// 		DataSource(const Udjat::Properties &node);
// 		virtual ~DataSource();

// 		/// @brief Build progress dialog for this source.
// 		std::shared_ptr<Udjat::Dialog::Progress> ProgressFactory() const;

// 		/// @brief True if datasource is a directory (ends with '/').
// 		bool dir() const;

// 		/// @brief Get path for source on local filesystem.
// 		virtual const char * local() const = 0;

// 		/// @brief Get path for source on remote filesystem.
// 		virtual const char * remote() const = 0;

// 		virtual bool has_remote() const noexcept;
// 		virtual bool has_local() const noexcept;

// 		/// @brief Get path for source on target image.
// 		virtual const char * path() const;

// 		/// @brief Get path relative to mount point.
// 		/// @return The path of file from mount point.
// 		const Udjat::String fspath() const;

// 		/// @brief Save source to file.
// 		/// @param path The destination file.
// 		virtual void save(const char *path);

// 		/// @brief Save source to temporary file.
// 		/// @return Path to local file.
// 		std::string save();

// 		/// @brief Save source, expand URL properties.
// 		/// @return Path to local file.
// 		virtual std::string save(const Udjat::Abstract::Object &object);

// 		/// @brief Save source.
// 		virtual void save(const std::function<bool(unsigned long long current, unsigned long long total, const void *buf, size_t length)> &writer);

// 		static bool for_each(const Udjat::URL &url, const std::function<bool(const DataSource &value)> &func);

// 		bool for_each(const std::function<bool(const char *filename)> &func) const;
// 		bool for_each(const std::function<bool(std::shared_ptr<DataSource> value)> &func) const;

// 		static void load(const Udjat::Properties &node, std::vector<std::shared_ptr<DataSource>> &sources, const char *nodename = nullptr);

// 		Udjat::URL url_local() const;

// 		/// @brief Get URL for remote source, resolving relative URLs and SLP repositories.
// 		/// @return The resolved URL for remote source.
// 		Udjat::URL url_remote() const;

// 	};

// 	/// @brief File based data source
// 	class UDJAT_API FileSource : public DataSource {
// 	protected:
// 		struct {
// 			const char *local = "";			///< @brief The URL for source in the local filesystem.
// 			const char *remote = "";		///< @brief The URL for source in the remote server.
// 			const char *path = "";			///< @brief Path for the file inside the destination image.
// 		} url;

// 		FileSource() {
// 		}

// 	public:
// 		FileSource(const char *path);
// 		FileSource(const Udjat::Properties &node, bool required = true);
// 		FileSource(const Udjat::Properties &node, const char *nodename, bool required = true);

// 		virtual ~FileSource();

// 		bool has_local() const noexcept override;
// 		bool has_remote() const noexcept override;

// 		// Expand string
// 		void expand(Udjat::String &str, const Udjat::Properties &node);

// 		// DataSource
// 		const char * local() const override;
// 		const char * remote() const override;
// 		const char * path() const override;

// 	};

// 	/// @brief Save source on temporary file
// 	class TempFileSource : public DataSource {
// 	private:
// 		std::string filename;		///< @brief The temporary file name.
// 		const std::string url;		///< @brief The URL for source in the remote server.
// 		const std::string filepath;	///< @brief Path for the file inside the destination image.

// 	public:

// 		TempFileSource(const DataSource &src);

// 		TempFileSource(const char *n, const std::string &u, const std::string &p);

// 		~TempFileSource();

// 		const char * local() const override;

// 		const char * remote() const override;

// 		const char * path() const override;

// 		std::string save(const Udjat::Abstract::Object &object) override;

// 		void save(const std::function<bool(unsigned long long current, unsigned long long total, const void *buf, size_t length)> &writer) override;

// 	};

//  }

//  namespace std {

// 	template <>
// 	struct hash<Reinstall::DataSource> {
// 		inline size_t operator()(const Reinstall::DataSource &obj) const {
// 			return std::hash<const char *>{}(obj.name());
// 		}
// 	};

// 	template <>
// 	struct equal_to<Reinstall::DataSource> {
// 		inline int operator()(const Reinstall::DataSource &lhs,const Reinstall::DataSource &rhs) const {
// 			return strcasecmp(lhs.name(),rhs.name()) == 0;
// 		}
// 	};

//  }

