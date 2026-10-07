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
  * @brief Implement GTK4 version of writer::open.
  */

 #define LOG_DOMAIN "writer"

 #include <config.h>
 #include <udjat/defs.h>
 #include <reinstall/tools/writer.h>
 #include <udjat/ui/status.h>
 #include <udjat/tools/intl.h>
 #include <semaphore.h>
 #include <gtkmm.h>
 #include <udjat/tools/logger.h>
 #include <private/gtkremovabledevicedialog.h>

 #include <private/toplevel.h>

 using namespace Udjat;
 using namespace std;

 bool TopLevel::open(const IsoWriter::Dialog &settings) {

	// Check for pre-selected output.
	if(Writer::open(settings)) {
		return true;
	}

	struct {
		string devdescr;
		string devname;
		int response = -1;
		sem_t semaphore;
	} info;

	sem_init(&info.semaphore,0,0);

	while(!*this) {

		Glib::signal_idle().connect([this,&info,&settings](){

			Udjat::Dialog::Status::getInstance().hide();

			auto *dialog = new GtkRemovableDeviceDialog(*this,settings);

#ifdef USE_MESSAGE_DIALOG
			dialog->signal_response().connect([dialog,&info](int rsp){
				info.response = rsp;
				info.devdescr = dialog->description();
				info.devname = dialog->device();
				Logger::String{"User selected '",info.devdescr,"' (",info.response,")"}.trace();
				sem_post(&info.semaphore);
				Udjat::Dialog::Status::getInstance().show();
				delete dialog;
			});
#else
			#error Needs implementation
#endif // USE_MESSAGE_DIALOG
			dialog->present();

			return 0;

		});

		sem_wait(&info.semaphore);

		if(info.response) {
			Logger::String{"Device selection dialog exits with rc=",info.response," (",strerror(info.response),")"}.warning("writer");
			Writer::close();
			throw runtime_error(strerror(info.response));
		}

		if(!*this) {

			try {

				debug("Opening ",info.devname.c_str());
				Writer::open(info.devname.c_str());

			} catch(const std::exception &e) {

				Logger::String{e.what()}.error("writer");
				Writer::close();
				throw;

			}

		}

	}

	this->device_url = info.devdescr.c_str();	
	Logger::String{"Writing image to '",this->device_url.strip().c_str(),"'"}.info();

	return true;
	
 }
