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
  * @brief Implements abstract image.
  */

 #define LOG_DOMAIN "iso9660"

 #include <config.h>
 #include <udjat/defs.h>
 #include <reinstall/tools/iso9660.h>
 #include <reinstall/tools/datasource.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/intl.h>
 #include <stdexcept>
 #include <libisofs/libisofs.h> 

 #ifdef HAVE_UNISTD_H
	#include <unistd.h>
 #endif // HAVE_UNISTD_H

 #include <cstdio>
 #include <cstdlib>
 #include <cstring>

 using namespace Udjat;
 using namespace std;

 namespace Iso9660 {

	namespace {

		struct SystemAreaPlan {
			int options = 0;
			int heads = 0;
			int secs = 0;
			int part_offset = 0;
		};

		/// @brief Options libisofs recognized in an imported system area.
		/// iso_image_get_system_area() returns the options that will be
		/// written, which are still 0 until we set them. The report is the
		/// mask xorriso -boot_image any replay applies.
		SystemAreaPlan read_system_area_plan(IsoImage *image) {

			SystemAreaPlan plan;
			char **lines = NULL;
			int count = 0;

			int rc = iso_image_report_system_area(image, &lines, &count, 0);
			if(rc < 0) {
				throw runtime_error(iso_error_to_msg(rc));
			}

			for(int i = 0; i < count; ++i) {
				const char *line = lines[i];
				const char *hit = strstr(line, "System area options:");
				if(hit) {
					plan.options = (int) strtol(hit + strlen("System area options:"), nullptr, 0);
				}

				hit = strstr(line, "MBR heads per cyl");
				if(hit && strchr(hit, ':')) {
					plan.heads = (int) strtol(strchr(hit, ':') + 1, nullptr, 0);
				}

				hit = strstr(line, "MBR secs per head");
				if(hit && strchr(hit, ':')) {
					plan.secs = (int) strtol(strchr(hit, ':') + 1, nullptr, 0);
				}

				hit = strstr(line, "Partition offset");
				if(hit && strchr(hit, ':')) {
					plan.part_offset = (int) strtol(strchr(hit, ':') + 1, nullptr, 0);
				}

				if(strstr(line, "MBR partition ") && !strstr(line, "table") && !strstr(line, "path")) {
					const char *colon = strchr(line, ':');
					int number = 0;
					int status = 0;
					if(colon && sscanf(colon + 1, " %d %i", &number, &status) == 2 && (status & 0x80)) {
						plan.options |= (1 << 15); // mbr_force_bootable
					}
				}
			}

			if(lines) {
				iso_image_report_system_area(image, &lines, &count, 1 << 15);
			}

			return plan;
		}

		/// @brief Re-arm boot-info-table patching on file-backed El Torito images.
		/// Patching an appended partition (no IsoFile) returns ISO_ISOLINUX_CANT_PATCH.
		void replay_boot_images(IsoImage *image, int sa_options) {

			int count = 0;
			ElToritoBootImage **boots = NULL;
			IsoFile **nodes = NULL;

			int rc = iso_image_get_all_boot_imgs(image, &count, &boots, &nodes, 0);
			if(rc <= 0 || count <= 0) {
				return;
			}

			iso_image_set_boot_catalog_hidden(
				image,
				LIBISO_HIDE_ON_RR | LIBISO_HIDE_ON_JOLIET | LIBISO_HIDE_ON_1999
			);

			for(int i = 0; i < count; ++i) {
				int opts = el_torito_get_isolinux_options(boots[i], 0);

				if(nodes[i] == NULL) {
					opts &= ~0x201;
				} else {
					if(el_torito_seems_boot_info_table(boots[i], 0)) {
						opts |= 0x1;
					} else {
						opts &= ~0x1;
					}
					if(el_torito_seems_boot_info_table(boots[i], 1)) {
						opts |= (1 << 9);
					} else {
						opts &= ~(1 << 9);
					}
					// xorriso replay: isolinux partition_entry=gpt_basdat on the EFI image.
					if((sa_options & 2) && el_torito_get_boot_platform_id(boots[i]) == 0xef) {
						opts = (opts & ~0xfc) | (1 << 2);
					}
				}

				el_torito_set_isolinux_options(boots[i], opts, 0);
			}

			free(boots);
			free(nodes);
		}

	}

	Controller::Controller() {
		Logger::String{"Initializing libisofs"}.info();
		if(!iso_init()) {
			throw runtime_error(_("Unexpected error initializing isofs"));
		}
		debug("Libisofs initialized");
	}

	Controller::~Controller() {
		Logger::String{"Deinitializing libisofs"}.trace();
		iso_finish();
	}

	Controller & Controller::get_instance() {
		static Controller instance;
		return instance;
	}


	Image::Image(const char *isoname) {

		int rc;
		Controller &cntrl = Controller::get_instance();

		rc = iso_image_new(PACKAGE_NAME, &image);
		if(rc < 0) {
			throw runtime_error(iso_error_to_msg(rc));
		}

		rc = iso_data_source_new_from_file(isoname, &dsrc);
		if(rc < 0) {
			throw runtime_error(iso_error_to_msg(rc));
		}

		rc = iso_read_opts_new(&ropts, 0);
		if(rc < 0) {
			throw runtime_error(iso_error_to_msg(rc));
		}

		// Same role as xorriso -indev plus -boot_image any replay: load the
		// tree, El Torito catalog and the 32 KiB system area.
		iso_read_opts_load_system_area(ropts, 1);
		iso_read_opts_keep_import_src(ropts, 1);

		{
			IsoReadImageFeatures *features = NULL;
			rc = iso_image_import(image, dsrc, ropts, &features);
			if(features) {
				iso_read_image_features_destroy(features);
			}
			if(rc < 0) {
				throw runtime_error(iso_error_to_msg(rc));
			}
		}

		iso_write_opts_new(&wopts, 2);

		memset(system_area, 0, sizeof(system_area));

		int ignored_options = 0;
		rc = iso_image_get_system_area(image, system_area, &ignored_options, 0);
		(void) ignored_options;
		if(rc < 0) {
			throw runtime_error(iso_error_to_msg(rc));
		}

		SystemAreaPlan plan;
		if(rc > 0) {
			plan = read_system_area_plan(image);

			// Bit1 patches the system area as an ISOLINUX isohybrid MBR.
			// libisofs returns ISO_ISOLINUX_CANT_PATCH ("Cannot patch isolinux
			// boot image") when that bit is set and no El Torito catalog was
			// loaded. A hardcoded 2 does that on every image.
			ElToritoBootImage *boot = NULL;
			if((plan.options & 2) && iso_image_get_boot_image(image, &boot, nullptr, nullptr) != 1) {
				Logger::String{"No El Torito catalog; not applying ISOLINUX isohybrid patching"}.warning();
				plan.options &= ~2;
			}

			if(plan.options & 2) {
				// zero_mbrpt: the isohybrid patch rebuilds the partition table.
				memset(system_area + 446, 0, 64);
			}

			rc = iso_write_opts_set_system_area(wopts, system_area, plan.options, 0);
			if(rc != ISO_SUCCESS) {
				throw runtime_error(iso_error_to_msg(rc));
			}

			if(plan.heads > 0 && plan.secs > 0) {
				rc = iso_write_opts_set_part_offset(wopts, (uint32_t) plan.part_offset, plan.secs, plan.heads);
				if(rc != ISO_SUCCESS) {
					throw runtime_error(iso_error_to_msg(rc));
				}
			}

			Logger::String{"Replaying system area options ", plan.options}.info();
		}

		replay_boot_images(image, plan.options);

		iso_write_opts_set_relaxed_vol_atts(wopts, 1);
		iso_write_opts_set_rrip_version_1_10(wopts, 1);
		iso_write_opts_set_rockridge(wopts, 1);
		iso_write_opts_set_joliet(wopts, 1);
		iso_write_opts_set_iso_level(wopts, 3);
		iso_write_opts_set_allow_deep_paths(wopts, 1);
		iso_write_opts_set_part_like_isohybrid(wopts, 1);

	}

	Image::~Image() {

		if(wopts) {
			iso_write_opts_free(wopts);
			wopts = nullptr;
		}

		if(ropts) {
			iso_read_opts_free(ropts);
			ropts = nullptr;
		}

		if(dsrc) {
			iso_data_source_unref(dsrc);
			dsrc = nullptr;
		}

		if(image) {
			iso_image_unref(image);
			image = nullptr;
		}

	}

	void Image::push_back(std::shared_ptr<Reinstall::DataSource::Item> source) {


	};

	void Image::write(const char *filename) {

		struct burn_source *src = NULL;
		unsigned char buf[2048];
		int fd, n, rc;

		rc = iso_image_update_sizes(image);
		if (rc < 0) {
			Logger::String{"Error updating image size: ",iso_error_to_msg(rc)}.error();
			throw runtime_error(iso_error_to_msg(rc));
		}

		rc = iso_image_create_burn_source(image, wopts, &src);
		if (rc < 0) {
			Logger::String{"Error creating burn source: ",iso_error_to_msg(rc)}.error();
			throw runtime_error(iso_error_to_msg(rc));
		}

		fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd < 0) {
			throw system_error(errno, system_category(),filename);
		}

		/* read_xt devolve blocos de 2048; o writer do reinstall faz o mesmo. */
		while ((n = src->read_xt(src, buf, sizeof buf)) == (int)sizeof buf) {
			if (::write(fd, buf, sizeof buf) != (int)sizeof buf) {
				throw runtime_error("Error writing data");	
			}
		}

		if (n < 0) {
			throw runtime_error("Error reading block");
		}

		close(fd);
		src->free_data(src);
		free(src);

	}


 }
