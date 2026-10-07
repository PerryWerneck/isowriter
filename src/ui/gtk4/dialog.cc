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
  * @brief Implements gtk4 dialogs.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <gtkmm.h>
 #include <udjat/tools/intl.h>
 #include <udjat/tools/logger.h>
 #include <reinstall/dialog.h>
 #include <private/toplevel.h>
 #include <udjat/tools/string.h>
 #include <udjat/tools/configuration.h>
 #include <memory>
 #include <glib/gi18n.h>

 using namespace Udjat;
 using namespace std;
 using namespace Gtk;

 std::shared_ptr<IsoWriter::Dialog> TopLevel::DialogFactory(const char *name, const Udjat::Properties &node, const char *msg, const IsoWriter::Dialog::Option buttons) {

	class Popup : public MessageDialog {
	public:

		// https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1Dialog.html

		Popup(const IsoWriter::Dialog &defs) : MessageDialog{""} {
			gtk_window_set_transient_for(
				GTK_WINDOW(gobj()),
				gtk_application_get_active_window(GTK_APPLICATION(g_application_get_default()))
			);
			set_modal(true);
		}

		~Popup() {
		}

	};

	class Dialog : public IsoWriter::Dialog {
	private:

		enum Responses {
			DIALOG_RESPONSE_NONE,
			DIALOG_RESPONSE_YES,
			DIALOG_RESPONSE_NO,
			DIALOG_RESPONSE_QUIT,
			DIALOG_RESPONSE_REBOOT,
			DIALOG_RESPONSE_CANCEL,
			DIALOG_RESPONSE_CONTINUE,
		};

	public:
		Dialog(const Udjat::Properties &node, const char *msg, const Option option) : IsoWriter::Dialog{node,msg,option} {
		}

		virtual ~Dialog() {
		}

		void quit() const override {
			Logger::String{"Finalizing the application"}.info("dialog");
			gtk_window_close(gtk_application_get_active_window(GTK_APPLICATION(g_application_get_default())));
		}

		void setup(MessageDialog &dialog) const {

			if(title && *title) {
				dialog.set_title(title);
			}

			gtk_window_set_transient_for(
				GTK_WINDOW(dialog.gobj()),
				gtk_application_get_active_window(GTK_APPLICATION(g_application_get_default()))
			);

			if(details && *details) {
				dialog.set_secondary_text(details);
			}

		}

		void add_buttons(MessageDialog &dialog) const {

			static const struct {
				Option option;
				const char *label;
				int response;
			} buttons[] = {
				{ Reboot, 		N_("_Reboot"), 				DIALOG_RESPONSE_REBOOT 		},
				{ Continue, 	N_("_Continue"), 			DIALOG_RESPONSE_CONTINUE 	},
				{ Quit, 		N_("_Quit application"),	DIALOG_RESPONSE_QUIT 		},
				{ Cancel, 		N_("C_ancel"), 				DIALOG_RESPONSE_CANCEL 		}
			};

			for(const auto &opt : this->buttons.order) {

				if((options & opt) == 0) {
					continue;
				}

				for(const auto &btn : buttons) {
					if(btn.option == opt) {
						debug("Adding button '",btn.label,"'");
						dialog.add_button(gettext(btn.label),btn.response);

						// Set button style classes
						// https://gnome.pages.gitlab.gnome.org/libadwaita/doc/main/style-classes.html

						auto button = dialog.get_widget_for_response(btn.response);
						auto style = button->get_style_context();
						
						if(this->buttons.destructive == opt) {
							style->add_class("destructive-action");
						} else if(this->buttons.destructive == opt) {
							style->add_class("suggested-action");
						}
						
					}
				}

			}

		}

		/// @brief Ask for confirmation.
		bool ask(bool default_response) const noexcept override {

			if(has(NonInteractive)) {
				return true;
			}

			auto mainloop = Glib::MainLoop::create();

			// https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1MessageDialog.html
			MessageDialog dialog{
				(message && *message) ? message : _("Do you want to continue?"),
				true,
				MessageType::QUESTION,
				ButtonsType::NONE,
				true
			};

			dialog.add_button(_("_No"),ResponseType::NO);
			dialog.add_button(_("_Yes"),ResponseType::YES);

			setup(dialog);

			if(destructive) {
				// https://gnome.pages.gitlab.gnome.org/libadwaita/doc/main/style-classes.html
				auto button = dialog.get_widget_for_response(ResponseType::YES);
				button->get_style_context()->add_class("destructive-action");
			}

			dialog.set_default_response(default_response ? ResponseType::YES : ResponseType::NO);

			bool rc = false;
			dialog.signal_response().connect([mainloop,&rc](int response){
				Logger::String{"Dialog response: ",response}.trace("gtk");
				rc = (response == ResponseType::YES);
				mainloop->quit();
			});

			dialog.present();
			mainloop->run();

			return rc;
		};
		
		/// @brief Show the dialog.
		bool present(const char *msg) const noexcept override {
			
			debug("Presenting dialog with message");

			auto str = make_shared<string>( (msg && *msg) ? msg : "" );

			Glib::signal_idle().connect_once([this,str](){

				if(IsoWriter::Dialog::present(str->c_str())) {
					// Already presented, no need to do anything else.
					return;
				}

				// https://gnome.pages.gitlab.gnome.org/gtkmm/classGtk_1_1MessageDialog.html
				auto *dialog = new MessageDialog(
					(message && *message) ? message : _("Do you want to continue?"),
					true,
					MessageType::INFO,
					ButtonsType::NONE,
					true
				);

				dialog->signal_response().connect([this,dialog](int response){

					switch(response) {
					case DIALOG_RESPONSE_QUIT:
						Logger::String{"The user response was 'quit'"}.info("dialog");
						quit();
						break;

					case DIALOG_RESPONSE_REBOOT:
						Logger::String{"The user response was 'reboot'"}.info("dialog");
						reboot();
						break;

					case DIALOG_RESPONSE_CANCEL:
						Logger::String{"The user response was 'cancel'"}.info("dialog");
						cancel();
						break;

					case DIALOG_RESPONSE_CONTINUE:
						Logger::String{"The user response was 'continue'"}.info("dialog");
						break;
					}

					delete dialog;

				});

				setup(*dialog);

				if(!str->empty()) {
					dialog->set_secondary_text(str->c_str());
				}

				add_buttons(*dialog);

				dialog->present();

			});

			return false; // No action was taken yet, the dialog is presented asynchronously.

		}

	};

	return make_shared<Dialog>(node,msg,buttons);
} 
