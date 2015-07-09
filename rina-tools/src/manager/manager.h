//
// Manager
//
// Addy Bombeke <addy.bombeke@ugent.be>
// Bernat Gastón <bernat.gaston@i2cat.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <string>
#include <librina/application.h>
#include <librina/cdap_v2.h>
#include "server.h"

static const unsigned int max_sdu_size_in_bytes = 10000;

class ManagerWorker : public ServerWorker {
public:
	ManagerWorker(rina::ThreadAttributes * threadAttributes,
	              rina::FlowInformation flow,
		      unsigned int max_sdu_size,
	              Server * serv);
	~ManagerWorker() throw() { };
	int internal_run();

private:
        void operate(rina::FlowInformation flow);
        void cacep(int port_id);
        bool createIPCP_1(int port_id);
        bool createIPCP_2(int port_id);
        bool createIPCP_3(int port_id);
        void queryRIB(int port_id, std::string name);

        static const std::string IPCP_1;
        static const std::string IPCP_2;
        static const std::string IPCP_3;
	rina::FlowInformation flow_;
	unsigned int max_sdu_size;
	rina::cdap::CDAPProviderInterface *cdap_prov_;
};

class Manager : public Server {
 public:
	Manager(const std::string& dif_name,
		const std::string& apn,
		const std::string& api);
	~Manager() { };
	void run(bool blocking);

 protected:
        void startWorker(rina::FlowInformation flow);
 private:
	static const std::string mad_name;
	static const std::string mad_instance;
	std::string dif_name_;
	bool client_app_reg_;
	rina::cdap_rib::con_handle_t con_;
 	ServerWorker* internal_start_worker_flow(rina::FlowInformation flow);
        ServerWorker* internal_start_worker(int port_id);
};
#endif//MANAGER_HPP
