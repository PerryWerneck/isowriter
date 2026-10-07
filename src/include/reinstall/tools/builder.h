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
  * @brief Declare abstract builder.
  */

 #pragma once
 #include <udjat/defs.h>
 #include <reinstall/action.h>
 #include <reinstall/tools/datasource.h>
 #include <udjat/tools/properties.h>
 #include <memory>
 #include <string>

 namespace IsoWriter {

	class UDJAT_API Builder : public IsoWriter::Action {
	public:

		/// @brief System kernel.
		class Kernel : public IsoWriter::DataSource {
		public:
			Kernel(const char *remote, const char *local = nullptr) : DataSource{remote,local} {		
			}

		};

		/// @brief System initrd
		class InitRD : public IsoWriter::DataSource {
		public:
			InitRD(const char *remote, const char *local = nullptr) : DataSource{remote,local} {		
			}

		};

		/// @brief Kernel parameter
		class KernelParameter {
		private:
			std::string kname;
			std::string kvalue;

		public:
			KernelParameter(const char *name, const char *value) : kname{name}, kvalue{value} {
			}
			
			KernelParameter(const Udjat::Properties &props) : kname{props["name"].c_str()}, kvalue{props["value"].c_str()} {
			}

			inline bool operator==(const char *n) const noexcept {
				return strcasecmp(kname.c_str(),n) == 0;
			}

			inline bool operator==(const std::string &n) const noexcept {
				return strcasecmp(kname.c_str(),n.c_str()) == 0;
			}

		};

		Builder(const char *name) : IsoWriter::Action{name} {
		};

		Builder(const Udjat::Properties &props);

		bool push_back(std::shared_ptr<IsoWriter::DataSource> source);
		bool push_back(std::shared_ptr<KernelParameter> kparm);

		/// @brief Load URLs for builder files.
		void load(std::vector<DataSource::Item> &itens);

		/// @brief Iterate from all builder files.
		/// @param task The tastk to be called on every file.
		/// @return true if the enumaration was interrupted by task 'true' return.
		// bool for_each(const std::function<bool(const Udjat::URL &from, const char *to)> &task) const;

	private:
		std::vector<std::shared_ptr<IsoWriter::DataSource>> sources;
		std::vector<std::shared_ptr<KernelParameter>> kparms;

	protected:
		std::shared_ptr<Kernel> kernel;
		std::shared_ptr<InitRD> initrd;

	};


 }

//  #include <reinstall/tools/datasource.h>
//  #include <reinstall/tools/kernelparameter.h>
//  #include <vector>
//  #include <memory>
//  #include <list>
//  #include <reinstall/image.h>
//  #include <reinstall/tools/efiboot.h>

//  namespace IsoWriter {

// 	class UDJAT_API Builder : public IsoWriter::Action {
// 	private:
// 		std::vector<std::shared_ptr<IsoWriter::DataSource>> sources;
// 		std::vector<std::shared_ptr<IsoWriter::Template>> templates;
// 		std::vector<std::shared_ptr<IsoWriter::KernelParameter>> kparms;

// 		/// @brief Append datasource in list, check for tempalte.
// 		void push_back(std::list<std::shared_ptr<DataSource>> &files, std::shared_ptr<DataSource> value);

// 		struct {
// 			const char *label = nullptr;
// 			std::string theme;
// 			std::shared_ptr<EFIBootImage> efi;	///> @brief EFI boot image settings.
// 		} boot;

// 	protected:
// 		std::shared_ptr<Dialog> output;

// 	public:
// 		Builder(const Udjat::Properties &node);
// 		virtual ~Builder();

// 		inline std::shared_ptr<EFIBootImage> efi() {
// 			return boot.efi;
// 		}

// 		bool getProperty(const char *key, std::string &value) const override;

// 		/// @brief Find template from filename.
// 		/// @return Valid template ptr if filename should be replaced.
// 		std::shared_ptr<IsoWriter::Template> tmplt(const char *filename);

// 		void prepare(std::list<std::shared_ptr<DataSource>> &files);

// 	};

//  }
