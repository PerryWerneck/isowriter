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
  * @brief Implements internal SLP client.
  */

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/tools/properties.h>
 #include <udjat/tools/object.h>
 #include <udjat/tools/intl.h>
 #include <udjat/ui/status.h>
 #include <mutex>
 #include <udjat/net/ip/address.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netdb.h>

 #ifdef HAVE_LIBSLP
	#include <slp.h>
 #endif // HAVE_LIBSLP

 #include <unistd.h>
 #include <list>

 #include <private/slpclient.h>

 using namespace Udjat;
 using namespace std;

 namespace IsoWriter {

	SLPClient::SLPClient(const Udjat::Properties &node) 
		: service_type{node["slp-service-type"].as_quark()},
		 	scope_list{node["slp-scope-list"].as_quark()},
		 	filter{node["slp-filter"].as_quark()},
			message{node.get("slp-search-message",service_type).as_quark()},
			allow_local{node.get("slp-allow-local",false)}
	{
	}

	bool SLPClient::operator==(const SLPClient &b) const noexcept {
		return service_type == b.service_type && scope_list == b.scope_list && filter == b.filter && allow_local == b.allow_local;
	}

	static std::shared_ptr<SLPClient> cache(std::shared_ptr<SLPClient> client) {

		static mutex guard;
		lock_guard<mutex> lock(guard);

		static list<std::shared_ptr<SLPClient>> clients;

		for(auto cached : clients) {
			if(*cached == *client) {
				return cached;
			}
		}

		clients.push_back(client);

		return client;

	}

	std::shared_ptr<SLPClient> SLPClient::Factory(const Udjat::Properties &node) {
		return cache(std::make_shared<SLPClient>(node));
	}

	void SLPClient::clear() noexcept {
		if(service_type && *service_type) {
			Logger::String{"Search for '",service_type,"' was disabled"}.info("slpclient");
		}
		service_type = "";
		scope_list = "";
		filter = "";
		query.url.clear();
		query.done = true;
	}

 #ifdef HAVE_LIBSLP

	struct SRV_URL_CB_INFO  {
		SLPError callbackerr;
		list<String> responses;
	};

	static SLPBoolean slpcallback( SLPHandle, const char* srvurl, unsigned short, SLPError errcode, void *cookie ) {

		// http://www.openslp.org/doc/html/ProgrammersGuide/SLPFindSrvs.html

		if(errcode == SLP_OK || errcode == SLP_LAST_CALL) {

			if(srvurl && *srvurl) {
				Logger::String{"Got response '",srvurl,"'"}.trace("slp");
				((SRV_URL_CB_INFO *) cookie)->responses.emplace_back(srvurl);
			}

			((SRV_URL_CB_INFO *) cookie)->callbackerr = SLP_OK;

		} else {

			Logger::String{"Got error '",errcode,"' on query"}.error("slp");
			((SRV_URL_CB_INFO *) cookie)->callbackerr = errcode;

		}

		return SLP_TRUE;

	}

	static void extract_url_from_response(const std::string &from, std::string &to) {

		const char *ptr = from.c_str();
		for(size_t ix = 0; ix < 2; ix++) {
			ptr = strchr(ptr,':');
			if(!ptr) {
				Logger::String{"Rejecting bad formatted response '",from,"'"}.warning("slp");
			}
			ptr++;
		}

		to.assign(ptr);

	}
#endif // HAVE_LIBSLP

	const char * SLPClient::url() {

		static mutex guard;
		lock_guard<mutex> lock(guard);

#ifdef HAVE_LIBSLP
		if(service_type && *service_type && !query.done) {

			auto &status = Dialog::Status::getInstance();
			status.busy(message);

			// https://github.com/ManageIQ/slp/blob/master/examples/raw_example.c
			// https://docs.oracle.com/cd/E19455-01/806-0628/6j9vie80v/index.html
			SLPError err;
			SLPHandle hSlp;

			err = SLPOpen(NULL, SLP_FALSE, &hSlp);
			if(err != SLP_OK) {
				Logger::String{"SLPOpen has failed"}.warning("slp");
				return "";
			}

			SRV_URL_CB_INFO cbinfo;

			Logger::String{"Searching for ",service_type}.info("slp");
			err = SLPFindSrvs(
						hSlp,
						service_type,
						scope_list,
						filter,
						slpcallback,
						&cbinfo
					);
			if(err != SLP_OK) {

				Logger::String{"SLPFindSrvs has failed"}.warning("slp");

			} else {

				size_t prefix_length = strlen(service_type);
				cbinfo.responses.remove_if([this,prefix_length](String &url){

					const char *str = url.c_str();
					if(strncmp(str,service_type,prefix_length)) {
						Logger::String{"Ignoring invalid response '",url,"'"}.warning("slp");
						return true;
					}

					str += prefix_length;
					if(*str != ':') {
						Logger::String{"Ignoring unexpected response '",url,"'"}.warning("slp");
						return true;
					}

					Logger::String{"Accepting valid response '",url,"'"}.trace("slp");

					return false;
				});

				if(cbinfo.responses.empty()) {

					Logger::String{"No SLP response for ",service_type}.warning("slp");

				} else {

					Logger::String{cbinfo.responses.size()," SLP response(s) for ",service_type}.info("slp");

					if(!allow_local) {

						// Ignore local addresses.

						vector<IP::Addresses> addresses; ///< @brief IP Addresses.
						IP::for_each([&addresses](const IP::Addresses &addr){
							addresses.push_back(addr);
							return false;
						});

						for(auto url=cbinfo.responses.begin(); url != cbinfo.responses.end() && this->query.url.empty(); url++) {

							SLPSrvURL *parsedurl = NULL;
							if(SLPParseSrvURL(url->c_str(),&parsedurl) != SLP_OK) {

								Logger::String{"Cant parse ",url->c_str()}.error("slp");

							} else if(parsedurl->s_pcHost && *parsedurl->s_pcHost) {

								struct addrinfo *ai;
								struct addrinfo hints;

								memset(&hints,0,sizeof(hints));
								hints.ai_family   = AF_UNSPEC;
								hints.ai_socktype = SOCK_STREAM;
								hints.ai_protocol = 0;
								hints.ai_flags    = AI_NUMERICSERV;

								string port;
								if(parsedurl->s_iPort) {
									port = std::to_string(parsedurl->s_iPort);
								}

								if(getaddrinfo(parsedurl->s_pcHost, port.c_str(), &hints, &ai)) {

									Logger::String{"Error resolving ",parsedurl->s_pcHost}.error("slp");

								} else {

									for(auto rp = ai;rp != NULL && this->query.url.empty(); rp = rp->ai_next) {

#if __cplusplus >= 201703L
										sockaddr_storage addr{IP::Factory(rp->ai_addr)};
#else
										sockaddr_storage addr = IP::Factory(rp->ai_addr);
#endif // C++17
										bool remote = true;

										for(IP::Addresses &local : addresses) {
											if(local.address == addr) {
												remote = false;
												Logger::String {"Ignoring response ",std::to_string(addr)}.trace("slpclient");
											}
										}

										if(remote) {
											extract_url_from_response(*url, this->query.url);
										}
									}

									freeaddrinfo(ai);
								}

								SLPFree(parsedurl);

							} else {

								Logger::String{"Ignoring response '",*url,"'"}.info("slp");

							}

						}

					} else {

						// Get first address.
						for(String &response : cbinfo.responses) {

							SLPSrvURL *parsedurl = NULL;
							if(SLPParseSrvURL(response.c_str(),&parsedurl) != SLP_OK) {

								Logger::String{"Cant parse ",response.c_str()}.error("slp");

							} else {

								extract_url_from_response(response, this->query.url);

								SLPFree(parsedurl);

								if(!this->query.url.empty()) {
									break;
								}
							}

						}

					}

				}

			}

			SLPClose(hSlp);
			status.busy(false);
			query.done = true;

		}
#endif // HAVE_LIBSLP

		return query.url.c_str();
	}

 }

