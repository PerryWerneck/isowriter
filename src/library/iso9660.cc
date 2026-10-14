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
  * @brief Implements abstract image.
  */

 #define LOG_DOMAIN "iso9660"

 #include <config.h>
 #include <udjat/defs.h>
 #include <reinstall/tools/iso9660.h>
 #include <reinstall/tools/datasource.h>
 #include <reinstall/tools/writer.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/intl.h>
 #include <stdexcept>
 #include <libisofs/libisofs.h> 
 #include <reinstall/dialog.h>

 #ifdef HAVE_UNISTD_H
	#include <unistd.h>
 #endif // HAVE_UNISTD_H

 #include <cstdio>
 #include <cstdlib>
 #include <cstring>
 #include <strings.h>
 #include <cerrno>
 #include <string>
 #include <system_error>
 #include <dirent.h>
 #include <sys/stat.h>

 using namespace Udjat;
 using namespace std;

 namespace Iso9660 {

	namespace {

		struct SystemAreaPlan {
			int options = 0;
			int heads = 0;
			int secs = 0;
			int part_offset = 0;
			unsigned long long image_size_512 = 0;
			bool saw_image_size = false;
			int mbr_count = 0;
			bool mbr_force_bootable_dummy = false;

			struct MbrPart {
				bool present = false;
				int type = -1;
			} mbr[4];

			/// @brief First 16 GPT partitions, index 0 is partition 1.
			struct GptPart {
				bool present = false;
				bool is_efi = false;
				bool has_path = false;
				bool saw_flags = false;
				bool fresh_efi = false;
				unsigned long long flags = 0;
				unsigned long long start = 0;
				unsigned long long blocks = 0;
			} gpt[16];

			/// @brief imported_iso interval of a path-less in-image ESP.
			string efi_interval;
		};

		/// @brief EFI System Partition type GUID, raw byte order.
		bool is_efi_type_guid(const char *guid) {
			return strcasecmp(guid, "28732ac11ff8d211ba4b00a0c93ec93b") == 0;
		}

		bool gpt_index(const char *line, int &number) {
			const char *colon = strchr(line, ':');
			if(!colon || sscanf(colon + 1, " %d", &number) != 1) {
				return false;
			}
			return number >= 1 && number <= 16;
		}

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
					unsigned int status = 0;
					unsigned int type = 0;
					unsigned long long start = 0;
					unsigned long long blocks = 0;
					int got = colon ? sscanf(colon + 1, " %d %i %i %llu %llu", &number, &status, &type, &start, &blocks) : 0;
					if(got >= 2 && (status & 0x80)) {
						plan.options |= (1 << 15); // mbr_force_bootable
					}
					if(got == 5 && number >= 1 && number <= 4) {
						plan.mbr[number - 1].present = true;
						plan.mbr[number - 1].type = (int) type;
						if(number > plan.mbr_count) {
							plan.mbr_count = number;
						}
						// Dummy entry libisofs uses to force the bootable flag.
						if(status == 0x80 && type == 0x00 && start == 0 && blocks == 1) {
							plan.mbr_force_bootable_dummy = true;
						}
					}
				}

				if(strstr(line, "ISO image size/512")) {
					const char *colon = strchr(line, ':');
					if(colon) {
						plan.image_size_512 = strtoull(colon + 1, nullptr, 10);
						plan.saw_image_size = true;
					}
				}

				int number = 0;
				if(strstr(line, "GPT type GUID") && gpt_index(line, number)) {
					char guid[80];
					const char *colon = strchr(line, ':');
					if(colon && sscanf(colon + 1, " %*d %79s", guid) == 1) {
						plan.gpt[number - 1].present = true;
						if(is_efi_type_guid(guid)) {
							plan.gpt[number - 1].is_efi = true;
						}
					}
				}

				if(strstr(line, "GPT partition flags:") && gpt_index(line, number)) {
					const char *hex = strstr(line, "0x");
					if(hex) {
						plan.gpt[number - 1].present = true;
						plan.gpt[number - 1].saw_flags = true;
						plan.gpt[number - 1].flags = strtoull(hex, nullptr, 0);
					}
				}

				if(strstr(line, "GPT start and size") && gpt_index(line, number)) {
					unsigned long long start = 0;
					unsigned long long blocks = 0;
					const char *colon = strchr(line, ':');
					if(colon && sscanf(colon + 1, " %*d %llu %llu", &start, &blocks) == 2) {
						plan.gpt[number - 1].present = true;
						plan.gpt[number - 1].start = start;
						plan.gpt[number - 1].blocks = blocks;
					}
				}

				if(strstr(line, "GPT partition path") && gpt_index(line, number)) {
					plan.gpt[number - 1].present = true;
					plan.gpt[number - 1].has_path = true;
				}
			}

			if(lines) {
				iso_image_report_system_area(image, &lines, &count, 1 << 15);
			}

			// xorriso sets gpt_iso_not_ro when GPT partition 1 lacks the
			// read-only flag (bit 60), even if that partition is the ESP.
			if(plan.gpt[0].present && plan.gpt[0].saw_flags && !(plan.gpt[0].flags & (1ULL << 60))) {
				plan.options |= (1 << 17);
			}

			return plan;
		}

		/// @brief Path-less EFI GPT partition that still lies inside the ISO.
		/// It is appended after the new image. Embedding it with efi_boot_part
		/// covers the ISO with Gap0, and that gap is what the guest mounts as
		/// /dev/sda1. An isohybrid image mentions the partition via gpt_basdat,
		/// which collides with a second EFI partition.
		void select_efi_boot_part(SystemAreaPlan &plan) {

			plan.efi_interval.clear();
			if(plan.options & 2) {
				return;
			}

			for(int i = 0; i < 16; ++i) {
				SystemAreaPlan::GptPart &part = plan.gpt[i];
				if(!part.present || !part.is_efi || part.has_path || part.blocks == 0) {
					continue;
				}
				if(!plan.saw_image_size || plan.image_size_512 < part.start + part.blocks) {
					continue;
				}

				char interval[96];
				snprintf(
					interval,
					sizeof(interval),
					"imported_iso:%llud-%llud::",
					part.start,
					part.start + part.blocks - 1
				);
				part.fresh_efi = true;
				plan.efi_interval = interval;
				return;
			}
		}

		/// @brief Keep the GRUB2 MBR code and drop the imported partition tables.
		/// Same bytes as grub2_mbr=--interval:imported_iso:0s-15s:zero_mbrpt,zero_gpt.
		/// The imported GPT still names the ESP at its old address.
		void keep_grub2_mbr_template(char system_area[32768]) {

			char head[8192];
			memcpy(head, system_area, sizeof(head));
			memset(system_area, 0, 32768);
			memcpy(system_area, head, sizeof(head));

			unsigned char *bytes = (unsigned char *) system_area;
			if(bytes[510] == 0x55 && bytes[511] == 0xaa) {
				memset(system_area + 446, 0, 64);
			}
			memset(system_area + 512, 0, 8192 - 512);
		}

		/// @brief GRUB's iso9660 UUID is the PVD modification date.
		/// The writer uses its own vol_uuid, not the timestamps imported with the image.
		void preserve_volume_uuid(IsoImage *image, IsoWriteOpts *opts) {

			char *creation = NULL;
			char *modification = NULL;
			char *expiration = NULL;
			char *effective = NULL;

			int rc = iso_image_get_pvd_times(image, &creation, &modification, &expiration, &effective);
			if(rc < 0 || !modification || !modification[0]) {
				return;
			}

			char uuid[17];
			int length = 0;
			while(length < 16 && modification[length] >= '0' && modification[length] <= '9') {
				uuid[length] = modification[length];
				++length;
			}
			uuid[length] = 0;
			if(length != 16) {
				return;
			}

			rc = iso_write_opts_set_pvd_times(opts, 0, 0, 0, 0, uuid);
			if(rc != ISO_SUCCESS) {
				throw runtime_error(iso_error_to_msg(rc));
			}
		}

		bool eltorito_lba_matches(IsoImage *image, unsigned long long start) {

			uint32_t lba = (uint32_t) (start / 4);
			int count = 0;
			ElToritoBootImage **boots = NULL;
			IsoFile **nodes = NULL;
			bool found = false;

			int rc = iso_image_get_all_boot_imgs(image, &count, &boots, &nodes, 0);
			if(rc > 0 && count > 0 && nodes) {
				for(int i = 0; i < count && !found; ++i) {
					if(!nodes[i]) {
						continue;
					}
					int sections_count = 0;
					struct iso_file_section *sections = NULL;
					rc = iso_file_get_old_image_sections(nodes[i], &sections_count, &sections, 0);
					if(rc > 0 && sections_count > 0 && sections && sections[0].block == lba) {
						found = true;
					}
					free(sections);
				}
			}

			free(boots);
			free(nodes);
			return found;
		}

		/// @brief Make GPT partition 1 a mountable ISO and put the ESP after it.
		/// libisofs only creates that ISO partition when the offset is 16.
		/// Gap fillers are left out: they are not filesystems, and mount
		/// reports a bad superblock for them.
		void publish_mountable_partitions(IsoWriteOpts *opts, string &efi_interval, int secs, int heads) {

			if(secs <= 0) {
				secs = 32;
			}
			if(heads <= 0) {
				heads = 64;
			}

			int rc = iso_write_opts_set_part_offset(opts, 16, secs, heads);
			if(rc != ISO_SUCCESS) {
				throw runtime_error(iso_error_to_msg(rc));
			}

			rc = iso_write_opts_set_partition_img(opts, 2, 0xef, efi_interval.data(), 1);
			if(rc != ISO_SUCCESS) {
				throw runtime_error(iso_error_to_msg(rc));
			}

			rc = iso_write_opts_set_appended_as_gpt(opts, 1);
			if(rc != ISO_SUCCESS) {
				throw runtime_error(iso_error_to_msg(rc));
			}

			rc = iso_write_opts_set_gpt_with_gaps(opts, 1, 0, 0);
			if(rc != ISO_SUCCESS) {
				throw runtime_error(iso_error_to_msg(rc));
			}
		}

		/// @brief part_like_isohybrid suppresses the protective MBR type 0xee.
		/// A real protective GPT needs that type so UEFI firmware finds the ESP.
		/// Isohybrid images already relied on the flag and keep it.
		bool part_like_isohybrid(IsoImage *image, const SystemAreaPlan &plan) {

			if(plan.options & 2) {
				return true;
			}

			bool real_protective = (plan.options & 1)
				&& plan.mbr[0].present
				&& plan.mbr[0].type == 0xee
				&& (plan.mbr_count == 1 || (plan.mbr_count == 2 && plan.mbr_force_bootable_dummy));
			if(real_protective) {
				return false;
			}

			for(int i = 0; i < 16; ++i) {
				const SystemAreaPlan::GptPart &part = plan.gpt[i];
				if(!part.present || part.fresh_efi || part.blocks == 0) {
					continue;
				}
				if(eltorito_lba_matches(image, part.start)) {
					return true;
				}
			}

			return false;
		}

		/// @brief Re-arm boot-info-table patching on BIOS El Torito images.
		/// Platform 0xef is the signed EFI filesystem; patching it breaks secure boot.
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
				int platform = el_torito_get_boot_platform_id(boots[i]);

				if(nodes[i] == NULL || platform == 0xef) {
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
				}

				// Isohybrid mentions the EFI image as a GPT basic-data partition.
				// efi_boot_part is used instead when that bit is off, and the two collide.
				if((sa_options & 2) && platform == 0xef) {
					opts = (opts & ~0xfc) | (1 << 2);
				}

				el_torito_set_isolinux_options(boots[i], opts, 0);
			}

			free(boots);
			free(nodes);
		}

		/// @brief True when path is a directory. lstat so a symlink is not one (-follow off).
		bool is_disk_directory(const char *path) {

			struct stat st;
			if(lstat(path, &st) < 0) {
				throw system_error(errno, system_category(), Logger::Message{_("Unable to stat '{}'"), path});
			}

			return S_ISDIR(st.st_mode);
		}

		void iso_check(int rc, const char *action, const char *path) {
			if(rc < 0) {
				Logger::String{"Error ",action," '",path,"': ",iso_error_to_msg(rc)}.error();
				throw runtime_error(iso_error_to_msg(rc));
			}
		}

		/// @brief Absolute ISO path. "." and repeated slashes are collapsed.
		/// ".." is rejected so the path cannot leave the image root.
		string normalize_iso_path(const char *path) {

			if(!path || !*path) {
				throw runtime_error(_("Missing ISO path for -map"));
			}

			const char *p = path;
			if(p[0] == '.' && (p[1] == '/' || p[1] == '\0')) {
				++p;
			}

			string out;
			while(*p) {
				while(*p == '/') {
					++p;
				}
				if(!*p) {
					break;
				}

				const char *start = p;
				while(*p && *p != '/') {
					++p;
				}

				string part(start, p);
				if(part.empty() || part == ".") {
					continue;
				}
				if(part == "..") {
					throw runtime_error(_("ISO path must not contain '..'"));
				}

				out.push_back('/');
				out += part;
			}

			return out.empty() ? string{"/"} : out;
		}

		void split_iso_leaf(const string &path, string &parent, string &leaf) {
			auto slash = path.rfind('/');
			if(slash == string::npos || slash == 0) {
				parent = "/";
				leaf = slash == 0 ? path.substr(1) : path;
				return;
			}
			parent = path.substr(0, slash);
			leaf = path.substr(slash + 1);
		}

		/// @brief Create missing parents. Attributes are copied from the
		/// parent, which is what libisofs does for implicit directories.
		IsoDir * ensure_iso_dir(IsoImage *image, const string &path) {

			IsoDir *dir = iso_image_get_root(image);
			if(path.empty() || path == "/") {
				return dir;
			}

			size_t index = path[0] == '/' ? 1 : 0;
			while(index < path.size()) {
				auto slash = path.find('/', index);
				string name = path.substr(index, slash == string::npos ? string::npos : slash - index);
				index = slash == string::npos ? path.size() : slash + 1;
				if(name.empty()) {
					continue;
				}

				IsoNode *node = nullptr;
				int rc = iso_image_dir_get_node(image, dir, name.c_str(), &node, 0);
				iso_check(rc, "looking up", name.c_str());

				if(rc != 1) {
					IsoDir *created = nullptr;
					rc = iso_image_add_new_dir(image, dir, name.c_str(), &created);
					iso_check(rc, "creating directory", name.c_str());
					dir = created;
					continue;
				}

				// -overwrite nondir: a file or symlink in the way of a new
				// directory is removed. An existing directory is kept.
				if(!ISO_NODE_IS_DIR(node)) {
					rc = iso_node_remove(node);
					iso_check(rc, "removing", name.c_str());
					IsoDir *created = nullptr;
					rc = iso_image_add_new_dir(image, dir, name.c_str(), &created);
					iso_check(rc, "creating directory", name.c_str());
					dir = created;
					continue;
				}

				dir = (IsoDir *) node;
			}

			return dir;
		}

		void map_into(IsoImage *image, IsoDir *parent, const char *leaf, const char *disk_path, const string &iso_path);

		/// @brief Insert the children of a disk directory. -map of a directory
		/// inserts the whole tree; two directories are merged.
		void map_children(IsoImage *image, IsoDir *parent, const char *disk_dir, const string &iso_dir) {

			// Closed on every exit, including a throw from map_into.
			struct Directory {
				DIR *handle;

				explicit Directory(const char *path) : handle{opendir(path)} {
					if(!handle) {
						throw system_error(errno, system_category(), Logger::Message{_("Unable to open directory '{}'"), path});
					}
				}

				~Directory() {
					closedir(handle);
				}

				Directory(const Directory &) = delete;
				Directory & operator=(const Directory &) = delete;
			} dir{disk_dir};

			for(;;) {

				errno = 0;
				dirent *entry = readdir(dir.handle);
				if(!entry) {
					if(errno != 0) {
						throw system_error(errno, system_category(), Logger::Message{_("Unable to read directory '{}'"), disk_dir});
					}
					break;
				}

				if(entry->d_name[0] == '.' && (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
					continue;
				}

				// Copy the name before the next readdir or a recursive walk.
				string name{entry->d_name};

				string child_disk{disk_dir};
				if(child_disk.empty() || child_disk.back() != '/') {
					child_disk.push_back('/');
				}
				child_disk += name;

				string child_iso = iso_dir == "/" ? "/" + name : iso_dir + "/" + name;
				map_into(image, parent, name.c_str(), child_disk.c_str(), child_iso);
			}

		}

		/// @brief xorriso -map with the default -overwrite "nondir".
		/// A directory is not replaced by a file. Anything else is removed
		/// and the disk object is inserted in its place. Disk symlinks are
		/// not followed (-follow off).
		void map_into(IsoImage *image, IsoDir *parent, const char *leaf, const char *disk_path, const string &iso_path) {

			const bool disk_dir = is_disk_directory(disk_path);

			IsoNode *old = nullptr;
			int rc = iso_image_dir_get_node(image, parent, leaf, &old, 0);
			iso_check(rc, "looking up", iso_path.c_str());

			if(rc == 1 && old && ISO_NODE_IS_DIR(old)) {
				if(!disk_dir) {
					throw runtime_error(Logger::Message{
						_("Refusing to replace ISO directory '{}'"),
						iso_path.c_str()
					}.c_str());
				}
				map_children(image, (IsoDir *) old, disk_path, iso_path);
				return;
			}

			if(rc == 1 && old) {
				rc = iso_node_remove(old);
				iso_check(rc, "removing", iso_path.c_str());
			}

			IsoNode *added = nullptr;
			rc = iso_tree_add_new_node(image, parent, leaf, disk_path, &added);
			iso_check(rc, "mapping", iso_path.c_str());

			if(disk_dir) {
				map_children(image, (IsoDir *) added, disk_path, iso_path);
			}

		}

		void map_disk_path(IsoImage *image, const char *disk_path, const string &iso_path) {

			if(iso_path == "/") {
				if(!is_disk_directory(disk_path)) {
					throw runtime_error(_("Cannot map a non-directory over '/'"));
				}
				map_children(image, iso_image_get_root(image), disk_path, "/");
				return;
			}

			string parent;
			string leaf;
			split_iso_leaf(iso_path, parent, leaf);
			if(leaf.empty()) {
				throw runtime_error(_("Invalid ISO path"));
			}

			map_into(image, ensure_iso_dir(image, parent), leaf.c_str(), disk_path, iso_path);
		}

	}

	// Controller::Controller() {
	// 	Logger::String{"Initializing libisofs"}.info();
	// 	if(!iso_init()) {
	// 		throw runtime_error(_("Unexpected error initializing isofs"));
	// 	}
	// 	debug("Libisofs initialized");
	// }

	// Controller::~Controller() {
	// 	Logger::String{"Deinitializing libisofs"}.trace();
	// 	iso_finish();
	// }

	// Controller & Controller::get_instance() {
	// 	static Controller instance;
	// 	return instance;
	// }


	Image::Image(const char *isoname) {

		int rc;
		// Controller::get_instance();

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
				Logger::String{"No El Torito catalog; not applying ISOLINUX isohybrid patching"}.info();
				plan.options &= ~2;
			}

			select_efi_boot_part(plan);

			if(!plan.efi_interval.empty()) {
				keep_grub2_mbr_template(system_area);
			} else if(plan.options & 2) {
				// zero_mbrpt: the isohybrid patch rebuilds the partition table.
				memset(system_area + 446, 0, 64);
			}

			rc = iso_write_opts_set_system_area(wopts, system_area, plan.options, 0);
			if(rc != ISO_SUCCESS) {
				throw runtime_error(iso_error_to_msg(rc));
			}

			if(!plan.efi_interval.empty()) {
				publish_mountable_partitions(wopts, plan.efi_interval, plan.secs, plan.heads);
			} else if(plan.heads > 0 && plan.secs > 0) {
				rc = iso_write_opts_set_part_offset(wopts, (uint32_t) plan.part_offset, plan.secs, plan.heads);
				if(rc != ISO_SUCCESS) {
					throw runtime_error(iso_error_to_msg(rc));
				}
			}

			Logger::String{
				"Replaying system area options ",
				plan.options,
				plan.efi_interval.empty() ? "" : " append_efi ",
				plan.efi_interval.c_str()
			}.info();
		}

		replay_boot_images(image, plan.options);
		preserve_volume_uuid(image, wopts);

		iso_write_opts_set_relaxed_vol_atts(wopts, 1);
		iso_write_opts_set_rrip_version_1_10(wopts, 1);
		iso_write_opts_set_rockridge(wopts, 1);
		iso_write_opts_set_joliet(wopts, 1);
		iso_write_opts_set_iso_level(wopts, 3);
		iso_write_opts_set_allow_deep_paths(wopts, 1);

		// -padding included: 150 blocks of 2 KiB stay inside the image.
		rc = iso_write_opts_set_tail_blocks(wopts, 150);
		if(rc != ISO_SUCCESS) {
			throw runtime_error(iso_error_to_msg(rc));
		}

		rc = iso_write_opts_set_part_like_isohybrid(wopts, part_like_isohybrid(image, plan) ? 1 : 0);
		if(rc != ISO_SUCCESS) {
			throw runtime_error(iso_error_to_msg(rc));
		}

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

	void Image::map(std::shared_ptr<IsoWriter::DataSource::Item> source) {

		if(!source) {
			throw invalid_argument(_("No source to map into the ISO image"));
		}

		if(source->path.empty()) {
			throw runtime_error(_("Missing ISO path for remap"));
		}

		auto disk_path = source->save();
		if(disk_path.empty()) {
			throw runtime_error(_("Unable to save the file to map into the ISO image"));
		}

		// libisofs reads disk_path when the burn source is created. Keep the
		// item alive so a temporary file survives until write().
		mapped.push_back(source);

		auto iso_path = normalize_iso_path(source->path.c_str());
		Logger::String{"Mapping '",disk_path.c_str(),"' onto '",iso_path.c_str(),"'"}.trace();
		map_disk_path(image, disk_path.c_str(), iso_path);

	};

	void Image::write() {

		struct burn_source *src = NULL;
		int rc;

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

		
		auto progress = IsoWriter::Progress::Factory();
		try {

			uint64_t image_size = src->get_size(src);
			auto writer = IsoWriter::Writer::get_instance(image_size);
			progress->url(writer->c_str());

			uint64_t current = 0;

			unsigned char buf[2048];
			while(src->read_xt(src, buf, sizeof buf) == (int)sizeof buf) {
				writer->write(current, (const char *) buf, sizeof buf);
				current += sizeof buf;
				if(progress->set(current,image_size)) {
					throw runtime_error(Logger::Message{_("Writing the ISO image to '{}' was cancelled"),writer->c_str()});
				}
			}

			progress->done();

			src->free_data(src);
			free(src);
			
		} catch(...) {

			progress->failed();

			src->free_data(src);
			free(src);

			throw;

		}

	}


 }
