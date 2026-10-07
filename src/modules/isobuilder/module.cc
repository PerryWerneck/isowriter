/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2021 Perry Werneck <perry.werneck@gmail.com>
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

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/module.h>
 #include <udjat/tools/properties.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/tools/properties.h>
 #include <reinstall/action.h>
 #include <udjat/tools/intl.h>
 #include <reinstall/tools/datasource.h>
 #include <reinstall/tools/writer.h>
 #include <reinstall/tools/template.h>
 #include <reinstall/tools/builder.h>
 #include <udjat/ui/status.h>
 #include <vector>
 #include <reinstall/modules/isobuilder.h>
 #include <list>
 #include <reinstall/modules/iso9660.h>
 #include <reinstall/modules/fatfs.h>
 #include <udjat/ui/status.h>
 #include <reinstall/application.h>

 #include <unistd.h>

 using namespace Udjat;
 using namespace std;

 namespace IsoWriter {

	/// @brief Base class for actions.
	class UDJAT_PRIVATE IsoBuilder::Module::Action : public IsoWriter::Builder {
	protected:
		virtual void build(list<std::shared_ptr<DataSource>> &files) = 0;

	public:
		Action(const Udjat::Properties &node)
			: IsoWriter::Builder{node} {

		}

		inline const char *name() const noexcept {
			return IsoWriter::Action::name();
		}

		void activate() override {

			list<std::shared_ptr<DataSource>> files;
			prepare(files);
			build(files);

			debug("IsoBuild action is complete");
			
		}

	};

	/// @brief ISO9660 builder.
#ifdef HAVE_LIBISOFS
	class UDJAT_PRIVATE Iso9660Builder : public IsoBuilder::Module::Action {
	private:
		std::shared_ptr<iso9660::Image::Settings> imgdef;

	protected:
		void build(list<std::shared_ptr<DataSource>> &files) override {

			// Build image ...
			auto &status = Udjat::Dialog::Status::getInstance();

			Logger::String{"Building ISO-9660 Image"}.info(name());
			status.sub_title(_("Building ISO-9660 Image"));
			status.state(_("Preparing image"));
	
			iso9660::Image image{this,imgdef};

			image.pre(*this);
			image.append(files);
			
			image.post(*this);

			// ... and write it to device.
			debug("Complete, writing...");
			image.write();
		}

	public:

		Iso9660Builder(const Udjat::Properties &node)
			: Action{node} {

			imgdef = make_shared<iso9660::Image::Settings>(node);
		}


	};
#endif // HAVE_LIBISOFS

	/// @brief Fat builder.
	class UDJAT_PRIVATE FatBuilder : public IsoBuilder::Module::Action {
	private:
		std::shared_ptr<FatFS::Image::Settings> imgdef;

	protected:
		void build(list<std::shared_ptr<DataSource>> &files) override {

			// Build image ...
			auto &status = Udjat::Dialog::Status::getInstance();

			debug("---------------------------------> FAT IMAGE");
			Logger::String{"Building Fat Image"}.info(name());
			status.sub_title(_("Building FAT Image"));

			FatFS::Image image{this,imgdef};

			image.pre(*this);
			image.append(files);
			image.post(*this);

			// ... and write it to device.
			debug("Complete, writing...");
			status.sub_title(_("Writing FAT Image"));
			image.write();

		}

	public:

		FatBuilder(const Udjat::Properties &node)
			: Action{node} {
			imgdef = make_shared<FatFS::Image::Settings>(node);
		}

	};

	IsoWriter::IsoBuilder::Module::Module(const char *name, const char *tagname) : Udjat::Module(name,"Build customized installation image."), Udjat::Properties::Parser{tagname} {
	}

	IsoWriter::IsoBuilder::Module::~Module() {
	}

	bool IsoWriter::IsoBuilder::Module::parse(const Udjat::Properties &props) {
		try {

			auto attr = props.get(
				"filesystem",
#if defined(HAVE_LIBISOFS)
				"iso9660"
#elif defined(HAVE_LIBFATFS)
				"fat32"
#else
				""
#endif // HAVE_LIBISOFS
			);

#ifdef HAVE_LIBISOFS
			if(strcasecmp(attr.c_str(),"iso9660") == 0) {
				IsoWriter::Application::getInstance().push_back(
					props,
					make_shared<Iso9660Builder>(props)
				);
				return true;
			}
#endif // HAVE_LIBISOFS

			if(strcasecmp(attr.c_str(),"fat") == 0 || strcasecmp(attr.c_str(),"fat32") == 0) {
				IsoWriter::Application::getInstance().push_back(
					props,
					make_shared<FatBuilder>(props)
				);
				return true;
			}

			Logger::String{"Unexpected value for attribute filesystem: '",attr.c_str(),"'"}.error(props.node_name());

		} catch(const std::exception &e) {

			Logger::String{e.what()}.error(props.node_name());

		}

		return false;

	}

	Udjat::Module * IsoWriter::IsoBuilder::Module::Factory(const char *name, const char *tagname) {
		return new Module(name,tagname);
	}


 }
