/*
 * RINA configuration tree
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef RINAD_RINA_CONFIGURATION_H
#define RINAD_RINA_CONFIGURATION_H

#ifdef __cplusplus

#include <librina/configuration.h>
#include <librina/common.h>

#include <string>
#include <sstream>
#include <list>
#include <map>


namespace rinad {

/*
 * Assigns an address prefix to a certain substring (the organization)
 * that is part of the application process name
 */
struct AddressPrefixConfiguration {

        static const unsigned int MAX_ADDRESSES_PER_PREFIX = 16;

        /*
         * The address prefix (it is the first valid address for the
         * given subdomain)
         */
        unsigned int addressPrefix;

        /* The organization whose addresses start by the prefix */
        std::string organization;

        AddressPrefixConfiguration() : addressPrefix(0) { }
};

struct NMinusOneFlowsConfiguration {

        /* The N-1 QoS id required by a management flow */
        int managementFlowQoSId;

        /*
         * The N-1 QoS id required by each N-1 flow  that has to be created
         * by the enrollee IPC Process after enrollment has been
         * successfully completed
         */
        std::list<int> dataFlowsQoSIds;

        NMinusOneFlowsConfiguration() : managementFlowQoSId(2) { }
};

/* The configuration of a known IPC Process */
struct KnownIPCProcessAddress {

        /* The application name of the remote IPC Process */
        rina::ApplicationProcessNamingInformation name;

        /* The address of the remote IPC Process */
        unsigned int address;

        KnownIPCProcessAddress() : address(0) { }
};

/* The configuration required to create a DIF */
struct DIFTemplate {
	std::string templateName;
        std::string difType;
        rina::DataTransferConstants dataTransferConstants;
        std::list<rina::QoSCube> qosCubes;
        rina::RMTConfiguration rmtConfiguration;
        rina::EnrollmentTaskConfiguration etConfiguration;
        rina::SecurityManagerConfiguration secManConfiguration;
        rina::FlowAllocatorConfiguration faConfiguration;
        rina::NamespaceManagerConfiguration nsmConfiguration;
        rina::ResourceAllocatorConfiguration raConfiguration;
        rina::RoutingConfiguration routingConfiguration;

        /*
         * The addresses of the known IPC Process (apname, address)
         * that can potentially be members of the DIFs I know
         */
        std::list<KnownIPCProcessAddress> knownIPCProcessAddresses;

        /* The address prefixes, assigned to different organizations */
        std::list<AddressPrefixConfiguration> addressPrefixes;

        /* Extra configuration parameters (name/value pairs) */
        std::map<std::string, std::string> configParameters;

        bool lookup_ipcp_address(
                        const rina::ApplicationProcessNamingInformation&,
                        unsigned int& result);

        std::string toString();
};

struct NeighborData {

        rina::ApplicationProcessNamingInformation apName;
        rina::ApplicationProcessNamingInformation supportingDifName;
        rina::ApplicationProcessNamingInformation difName;
};


/* The configuration required to create an IPC Process */
struct IPCProcessToCreate {

        rina::ApplicationProcessNamingInformation name;
        rina::ApplicationProcessNamingInformation difName;
        std::list<NeighborData> neighbors;
        std::list<rina::ApplicationProcessNamingInformation> difsToRegisterAt;
        std::string hostname;
        /*
         * Specifies what SDU Protection Module should be used for each possible
         * N-1 DIF (being NULL the default one)
         * nMinus1DIFName: sduProtectionType
         */
        std::map<std::string, std::string> sduProtectionOptions;
        std::map<std::string, std::string> parameters;
};

/* Configuration of the local RINA Software instantiation */
struct LocalConfiguration {

        /*
         * The port where the IPC Manager is listening for incoming local
         * TCP connections from administrators
         */
        int consolePort;

        /* The path to the RINA binaries installation in the system */
        std::string installationPath;

        /* The path to the RINA libraries in the system */
        std::string libraryPath;

        /* The path to the RINA log folder in the system */
        std::string logPath;

	/* The paths where to look for policy plugins. */
	std::list<std::string> pluginsPaths;

        std::string toString() const;

        LocalConfiguration() :
                        consolePort(32766){ }
};

struct DIFTemplateMapping {

	// The name of the DIF
	rina::ApplicationProcessNamingInformation dif_name;

	// The name of the template
	std::string template_name;
};

/*
 * Global configuration for the RINA software
 */
class RINAConfiguration {
 public:
        /*
         * The local software configuration (console port, timeouts, ...)
         */
        LocalConfiguration local;

        /*
         * The list of IPC Process to create when the software starts
         */
        std::list<IPCProcessToCreate> ipcProcessesToCreate;

        /*
         * The configurations of zero or more DIFs
         */
        std::list<DIFTemplateMapping> difConfigurations;

        /*
         * Application to DIF mappings
         *
         * std::string encodedAppName:
         *  The application name encoded this way:
         *  APName-APInstance-AEName-AEInstance
         *
         * rina::ApplicationProcessNamingInformation difName;
         */
        std::map<std::string,
                rina::ApplicationProcessNamingInformation>
                applicationToDIFMappings;

	/*
	 * The path of the configuration file where the configuration
	 * comes from
	 */
	std::string configuration_file;

        bool lookup_dif_template_mappings(
                        const rina::ApplicationProcessNamingInformation& dif_name,
                        DIFTemplateMapping& result) const;

        bool lookup_dif_by_application(
                const rina::ApplicationProcessNamingInformation& app_name,
                rina::ApplicationProcessNamingInformation& result);

#if 0
        bool lookup_ipcp_address(const std::string dif_name,
                const rina::ApplicationProcessNamingInformation& ipcp_name,
                unsigned int& result);
#endif
        std::string toString() const;
};

}

#endif

#endif
