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

 #ifdef LOG_DOMAIN
	#undef LOG_DOMAIN
 #endif // LOG_DOMAIN
 #define LOG_DOMAIN "grub2"
 #include <udjat/tools/logger.h>

 #include <udjat/tools/string.h>
 #include <udjat/tools/configuration.h>
 #include <udjat/tools/intl.h>
 #include <udjat/ui/status.h>
 #include <reinstall/application.h>
 #include <reinstall/action.h>
 #include <reinstall/dialog.h>
 #include <reinstall/group.h>
 #include <reinstall/tools/datasource.h>
 #include <reinstall/tools/kernelparameter.h>
 #include <reinstall/tools/template.h>
 #include <reinstall/tools/script.h>
 #include <string>
 #include <udjat/tools/properties.h>

 #include <reinstall/modules/grub2.h>

 using namespace Udjat;
 using namespace std;

 static const char * PathFactory(const Udjat::Abstract::Object &object, const Udjat::Properties &node, const char *name, const char *text) {

	String str{text};

	str.expand('$',[&](const char *key, std::string &value){

		if(!strcasecmp(key,"name")) {
			value = name;
			return true;
		}

		if(!strcasecmp(key,"filename")) {
			value = Config::Value<string>("app-defaults",(string{name}+"-name").c_str(),"${name}.reinstall");
			return true;
		}

		return false;
	});

	str.expand(node);
	str.expand(object);

	return str.as_quark();
 }

 namespace IsoWriter {

	class UDJAT_PRIVATE Grub2::Module::Action : public IsoWriter::Action {
	private:

		class Kernel : public IsoWriter::FileSource {
		public:
			Kernel(const Udjat::Abstract::Object &object, const Udjat::Properties &node) : FileSource{node,"kernel"} {
				url.local = ::PathFactory(object,node,"kernel","file://${boot.path.mount}${boot.path.relative}/${filename}");
				url.path = ::PathFactory(object,node,"kernel","${boot.path.relative}/${filename}");

				if(!(message && *message)) {
					message = _("Getting installation kernel");
				}
			}
		};

		class Init : public IsoWriter::FileSource {
		public:
			Init(const Udjat::Abstract::Object &object, const Udjat::Properties &node) : FileSource{node,"init"} {
				url.local = ::PathFactory(object,node,"initrd","file://${boot.path.mount}${boot.path.relative}/${filename}");
				url.path = ::PathFactory(object,node,"initrd","${boot.path.relative}/${filename}");

				if(!(message && *message)) {
					message = _("Getting init system");
				}
			}
		};

		class DUD : public IsoWriter::FileSource {
			public:
				DUD(const Udjat::Properties &node, const char *path) : FileSource{node} {
					url.local = path;
				}

		};

		std::vector<std::shared_ptr<IsoWriter::DataSource>> sources;
		std::vector<std::shared_ptr<IsoWriter::KernelParameter>> kparms;
		std::vector<std::shared_ptr<IsoWriter::Template>> templates;
		std::vector<std::shared_ptr<IsoWriter::Script>> scripts;

		const char *boot_label = nullptr;

	public:
		Action(const Udjat::Properties &node) : IsoWriter::Action{node} {

			static const char *labels[] = {
				"grub-label",
				"boot-label",
				"label",
				"system-name"
			};

			for(const char *label : labels) {
				const char *ptr = node[label].as_quark();
				if(ptr && *ptr) {
					boot_label = ptr;
					Logger::String{"Setting boot-label to '",boot_label,"' from attribute '",label,"'"}.trace(name());
					break;
				}
			}

			if(!(boot_label && *boot_label)) {
				String label{Config::Value<string>{"defaults","boot-label",_("Reinstall workstation")}.c_str()};
				label.expand(node);
				boot_label = label.as_quark();
				Logger::String{"Required attribute 'boot-label' is missing or invalid, using default '",boot_label,"'"}.warning(name());
			}

			sources.push_back(make_shared<Kernel>(*this,node));
			sources.push_back(make_shared<Init>(*this,node));

			node.for_each_child("driver-update-disk",[this](const Properties &property){
				auto path = property["path"];
				if(!path.empty()) {
					auto source = make_shared<DUD>(property,path.as_quark());
					sources.push_back(source);
				}
				return false;
			});

			// for(Udjat::Properties nd = node; nd; nd = nd.parent()) {
			// 	for(Udjat::Properties child = nd.child("driver-update-disk"); child; child = child.next_sibling("driver-update-disk")) {
			// 		auto path = XML::QuarkFactory(child,"path");
			// 		if(path && *path) {
			// 			auto source = make_shared<DUD>(child,path);
			// 			sources.push_back(source);
			// 		}
			// 	}
			// }

			// Load kernel parameters.
			IsoWriter::KernelParameter::load(node,kparms,true);

			// Load templates
			IsoWriter::Template::load(*this,node,templates);

			// Load scripts.
			IsoWriter::Script::load(*this,node,scripts);

			// Enable allow reboot on success dialog.
			success->set(Dialog::Reboot);

		}

		const Kernel * kernel() const {
			for(const auto &source : sources) {
				const Kernel *object = dynamic_cast<Kernel *>(source.get());
				if(object) {
					return object;
				}
			}
			throw logic_error("Unable to find installation kernel");
		}

		const Init * init() const {
			for(const auto &source : sources) {
				const Init *object = dynamic_cast<Init *>(source.get());
				if(object) {
					return object;
				}
			}
			throw logic_error("Unable to find installation system");
		}

		bool getProperty(const char *key, std::string &value) const override {

			if(!strcasecmp(key,"grub-config")) {
#ifdef DEBUG
				value = "/tmp/grub.cfg";
#else
				value = "/boot/grub2/grub.cfg";
#endif // DEBUG
				return true;
			}

			if(!(strcasecmp(key,"boot-kernel"))) {
				value = kernel()->fspath();
				debug(key,"='",value.c_str(),"'");
				return true;
			}

			if(!(strcasecmp(key,"boot-initrd") )) {
				value = init()->fspath();
				debug(key,"='",value.c_str(),"'");
				return true;
			}

			if(!(strcasecmp(key,"kernel-file") && strcasecmp(key,"kernel-filename"))) {
				const Kernel *object = kernel();
				const char *ptr = strrchr(object->local(),'/');
				if(ptr) {
					value = ptr+1;
					debug(key,"='",value.c_str(),"'");
					return true;
				}
			}

			if(!(strcasecmp(key,"initrd-file") && strcasecmp(key,"initrd-filename"))) {
				const Init *object = init();
				const char *ptr = strrchr(object->local(),'/');
				if(ptr) {
					value = ptr+1;
					debug(key,"='",value.c_str(),"'");
					return true;
				}
			}

			if(!(strcasecmp(key,"boot-label") && strcasecmp(key,"install-label"))) {

				if(boot_label && *boot_label) {
					value = boot_label;
				} else {
					value = Config::Value<string>{"defaults","boot-label",_("Reinstall workstation")};
				}
				return true;
			}

			if(!strcasecmp(key,"kernel-parameters")) {
				value = KernelParameter::join(*this,kparms);
				debug("Kernel parameters set to '",value.c_str(),"'");
				return true;
			}

			if(!strcasecmp(key,"grub-conf-dir")) {
#ifdef DEBUG
				value = "/tmp/";
				debug("Grub config was set to '",value.c_str(),"'");
#else
				value = Config::Value<string>("grub","conf-dir","/etc/grub.d/");
#endif // DEBUG
				return true;
			}

			return IsoWriter::Action::getProperty(key,value);
		}

		void activate() override {

			auto &status = Udjat::Dialog::Status::getInstance();
			status.sub_title(_("Getting required files"));
			for(const auto &source : sources) {
				source->save(*this);
			}

			Logger::String{"Applying templates"}.info(name());
			{
				status.sub_title(_("Applying templates"));
				for(const auto &tmplt : templates) {
					auto progress = Udjat::Dialog::Progress::getInstance();
					progress->url(tmplt->to_string());
					tmplt->save(*this,[progress](uint64_t current, uint64_t total) {
						progress->set(current,total);
						return false;
					});
					progress->done();
				}
			}

			Logger::String{"Configuring boot loader"}.info(name());
			status.sub_title(_("Configuring boot loader"));
			for(auto &script : scripts) {
				script->run(*this,Script::Pre,_("First stage"));
			}
			for(auto &script : scripts) {
				script->run(*this,Script::Post,_("Second stage"));
			}

		}

	};
	

	Grub2::Module::Module(const char *name) : Udjat::Module(name,"Reinstallation without disk image."), Udjat::Properties::Parser("local-installer") {
	};

	Grub2::Module::~Module() {
	}	

	// Udjat::Properties::Parser interface.
	bool Grub2::Module::parse(const Udjat::Properties &node) {
		// Logger::String{"Building action '",node.attribute("name").as_string(),"' from '",node.path(),"'"}.info("isowriter");
		IsoWriter::Application::getInstance().push_back(node,make_shared<Grub2::Module::Action>(node));
		return true;
	}

	Udjat::Module * Grub2::Module::Factory(const char *name) {
		return new Grub2::Module(name);
	}

 }
