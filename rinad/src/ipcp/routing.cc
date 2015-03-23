//
// Routing component
//
//    Eduard Grasa <eduard.grasa@i2cat.net>
//    Vincenzo Maffione <v.maffione@nextworks.it>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <assert.h>

#define RINA_PREFIX "routing"

#include <librina/logs.h>

#include "ipcp/components.h"

namespace rinad {

//Class RoutingComponent
RoutingComponent::RoutingComponent() {
	ipcp = 0;
}

void RoutingComponent::set_ipc_process(IPCProcess * ipc_process)
{
	ipcp = ipc_process;
}

void RoutingComponent::set_dif_configuration(const rina::DIFConfiguration& dif_configuration) {
	if (!ps) {
		return;
	}

	IRoutingPs *rps = dynamic_cast<IRoutingPs *> (ps);
	assert(rps);
	rps->set_dif_configuration(dif_configuration);
}

int RoutingComponent::select_policy_set(const std::string& path,
                                       const std::string& name)
{
        return select_policy_set_common(ipcp, "routing", path, name);
}

int RoutingComponent::set_policy_set_param(const std::string& path,
                                          const std::string& name,
                                          const std::string& value)
{
        return set_policy_set_param_common(ipcp, path, name, value);
}

}
