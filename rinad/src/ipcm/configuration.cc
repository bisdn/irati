/*
 * Configuration reader for IPC Manager
 *
 *    Sander Vrijders       <sander.vrijders@intec.ugent.be>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>

#define RINA_PREFIX "ipcm.conf"

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/logs.h>
#include <librina/json/json.h>
#include "ipcm.h"

using namespace std;

namespace rinad {

void parse_name(const Json::Value &root,
                rina::ApplicationProcessNamingInformation &name)
{
        name.processName     = root.get("apName",     string()).asString();
        name.processInstance = root.get("apInstance", string()).asString();
        name.entityName      = root.get("aeName",     string()).asString();
        name.entityInstance  = root.get("aeInstance", string()).asString();
}

void parse_policy(const Json::Value  & root,
                  const string       & name,
                  rina::PolicyConfig & pol)
{
        Json::Value p = root[name];

        if (p == 0)
        	return;

        pol.name_    = p.get("name", string()).asString();
        pol.version_ = p.get("version", string()).asString();

        Json::Value par = p["parameters"];
        if (par != 0) {
                for (unsigned int i = 0; i < par.size(); i++) {
                        rina::PolicyParameter pp;

                        pp.name_  = par[i].get("name",  string()).asString();
                        pp.value_ = par[i].get("value", string()).asString();
                        pol.parameters_.push_back(pp);
                }
        }
}

void parse_flow_ctrl(const Json::Value  root,
                     rina::DTCPConfig & dc)
{
        Json::Value flow_ctrl = root["flowControlConfig"];

        if (flow_ctrl != 0) {
                rina::DTCPFlowControlConfig fc;

                fc.window_based_ =
                        flow_ctrl.get("windowBased",fc.window_based_).asBool();

                // window_based_config_
                Json::Value w_flow_ctrl = flow_ctrl["windowBasedConfig"];
                if (w_flow_ctrl != 0) {
                        rina::DTCPWindowBasedFlowControlConfig wfc;

                        wfc.max_closed_window_queue_length_ = w_flow_ctrl
                                .get("maxClosedWindowQueueLength",
                                     wfc.max_closed_window_queue_length_)
                                .asUInt();

                        wfc.initial_credit_ = w_flow_ctrl
                                .get("initialCredit",
                                     wfc.initial_credit_)
                                .asUInt();

                        parse_policy(w_flow_ctrl,
                                     "rcvrFlowControlPolicy",
                                     wfc.rcvr_flow_control_policy_);

                        parse_policy(w_flow_ctrl,
                                     "txControlPolicy",
                                     wfc.tx_control_policy_);

                        fc.window_based_config_ = wfc;
                }

                fc.rate_based_ =
                        flow_ctrl.get("rateBased",
                                      fc.rate_based_).asBool();
                // rate_based_config_
                Json::Value r_flow_ctrl = flow_ctrl["rateBasedConfig"];
                if (r_flow_ctrl != 0) {
                        rina::DTCPRateBasedFlowControlConfig rfc;

                        rfc.sending_rate_ = r_flow_ctrl
                                .get("sendingRate", rfc.sending_rate_)
                                .asUInt();

                        rfc.time_period_ = r_flow_ctrl
                                .get("timePeriod", rfc.time_period_)
                                .asUInt();

                        parse_policy(r_flow_ctrl,
                                     "noRateSlowDownPolicy",
                                     rfc.no_rate_slow_down_policy_);

                        parse_policy(r_flow_ctrl,
                                     "noOverrideDefaultPeakPolicy",
                                     rfc.no_override_default_peak_policy_);

                        parse_policy(r_flow_ctrl,
                                     "rateReductionPolicy",
                                     rfc.rate_reduction_policy_);

                }

                fc.sent_bytes_threshold_ = flow_ctrl
                        .get("sentBytesThreshold", fc.sent_bytes_threshold_)
                        .asInt();

                fc.sent_bytes_percent_threshold_ = flow_ctrl
                        .get("sentBytesPercentThreshold",
                             fc.sent_bytes_percent_threshold_)
                        .asInt();

                fc.sent_buffers_threshold_ = flow_ctrl
                        .get("sentBuffersThreshold",
                             fc.sent_buffers_threshold_)
                        .asInt();

                fc.rcv_bytes_threshold_ = flow_ctrl
                        .get("rcvBytesThreshold", fc.rcv_bytes_threshold_)
                        .asInt();

                fc.rcv_bytes_percent_threshold_ = flow_ctrl
                        .get("rcvBytesThreshold",
                             fc.rcv_bytes_percent_threshold_)
                        .asInt();

                fc.rcv_buffers_threshold_ = flow_ctrl
                        .get("rcvBuffersThreshold", fc.rcv_buffers_threshold_)
                        .asInt();

                parse_policy(flow_ctrl,
                             "closedWindowPolicy",
                             fc.closed_window_policy_);

                parse_policy(flow_ctrl,
                             "flowControlOverrunPolicy",
                             fc.flow_control_overrun_policy_);

                parse_policy(flow_ctrl,
                             "reconcileFlowControlPolicy",
                             fc.reconcile_flow_control_policy_);

                parse_policy(flow_ctrl,
                             "receivingFlowControlPolicy",
                             fc.receiving_flow_control_policy_);

                dc.flow_control_config_ = fc;
        }
}

void parse_rtx_flow_ctrl(const Json::Value  root,
                         rina::DTCPConfig & dc)
{
        Json::Value rtx_ctrl = root["rtxControlConfig"];
        if (rtx_ctrl != 0) {
                rina::DTCPRtxControlConfig rfc;

                rfc.max_time_to_retry_ =
                        rtx_ctrl.get("maxTimeToRetry",
                                     rfc.max_time_to_retry_)
                        .asUInt();

                rfc.data_rxms_nmax_ =
                        rtx_ctrl.get("dataRxmsNmax",
                                     rfc.data_rxms_nmax_)
                        .asUInt();

                rfc.initial_rtx_time_ =
                        rtx_ctrl.get("initialRtxTime",
                                     rfc.initial_rtx_time_)
                        .asUInt();

                parse_policy(rtx_ctrl,
                             "rtxTimerExpiryPolicy",
                             rfc.rtx_timer_expiry_policy_);

                parse_policy(rtx_ctrl,
                             "senderAckPolicy",
                             rfc.sender_ack_policy_);

                parse_policy(rtx_ctrl,
                             "recvingAckListPolicy",
                             rfc.recving_ack_list_policy_);

                parse_policy(rtx_ctrl,
                             "rcvrAckPolicy",
                             rfc.rcvr_ack_policy_);

                parse_policy(rtx_ctrl,
                             "sendingAckPolicy",
                             rfc.sending_ack_policy_);

                parse_policy(rtx_ctrl,
                             "rcvrControlAckPolicy",
                             rfc.rcvr_control_ack_policy_);

                dc.rtx_control_config_ = rfc;
        }
}

void parse_efcp_policies(const Json::Value  root,
                         rina::QoSCube    & cube)
{
        Json::Value con_pol = root["efcpPolicies"];
        if (con_pol != 0) {
                rina::ConnectionPolicies cp;

                cp.dtcp_present_ = con_pol.get("dtcpPresent",
                                               cp.dtcp_present_).asBool();

                parse_policy(con_pol,
                             "dtpPolicySet",
                             cp.dtp_policy_set_);

                // DTCPConfig
                Json::Value dtcp_conf = con_pol["dtcpConfiguration"];
                if (dtcp_conf != 0) {
                        rina::DTCPConfig dc;

                        dc.flow_control_ =
                                dtcp_conf.get("flowControl",
                                              dc.flow_control_).asBool();
                        // flow_control_config_
                        parse_flow_ctrl(dtcp_conf, dc);

                        dc.rtx_control_ =
                                dtcp_conf.get("rtxControl",
                                              dc.rtx_control_).asBool();

                        // rtx_control_config_
                        parse_rtx_flow_ctrl(dtcp_conf, dc);

                        parse_policy(dtcp_conf,
                                     "dtcpPolicySet",
                                     dc.dtcp_policy_set_);

                        parse_policy(dtcp_conf,
                                     "lostControlPduPolicy",
                                     dc.lost_control_pdu_policy_);

                        parse_policy(dtcp_conf,
                                     "rttEstimatorPolicy",
                                     dc.rtt_estimator_policy_);

                        cp.dtcp_configuration_ = dc;
                }

                parse_policy(dtcp_conf,
                             "rcvrTimerInactivityPolicy",
                             cp.rcvr_timer_inactivity_policy_);

                parse_policy(dtcp_conf,
                             "senderTimerInactivityPolicy",
                             cp.sender_timer_inactivity_policy_);

                parse_policy(con_pol,
                             "initialSeqNumPolicy",
                             cp.initial_seq_num_policy_);

                cp.seq_num_rollover_threshold_ =
                        con_pol.get("seqNumRolloverThreshold",
                                    cp.seq_num_rollover_threshold_).asUInt();
                cp.initial_a_timer_ =
                        con_pol.get("initialATimer",
                                    cp.initial_a_timer_).asUInt();
                cp.partial_delivery_ =
                        con_pol.get("partialDelivery",
                                    cp.partial_delivery_).asBool();
                cp.incomplete_delivery_ =
                        con_pol.get("incompleteDelivery",
                                    cp.incomplete_delivery_).asBool();
                cp.in_order_delivery_ =
                        con_pol.get("inOrderDelivery",
                                    cp.in_order_delivery_).asBool();
                cp.max_sdu_gap_ =
                        con_pol.get("maxSduGap", cp.max_sdu_gap_).asInt();

                cube.efcp_policies_ = cp;
        }
}

void parse_ipc_to_create(const Json::Value          root,
                         list<IPCProcessToCreate> & ipcProcessesToCreate)
{
        Json::Value ipc_processes = root["ipcProcessesToCreate"];

        for (unsigned int i = 0; i < ipc_processes.size(); i++) {
                rinad::IPCProcessToCreate ipc;

                // IPC process Names
                // Might want to move this to another function
                parse_name(ipc_processes[i], ipc.name);

                ipc.difName = rina::ApplicationProcessNamingInformation
                        (ipc_processes[i].get
                         ("difName", string()).asString(),
                         string());

                //Neighbors
                Json::Value neigh = ipc_processes[i]["neighbors"];
                if (neigh != 0) {
                        for (unsigned int j = 0; j < neigh.size(); j++) {
                                rinad::NeighborData neigh_data;

                                parse_name(neigh[j], neigh_data.apName);

                                neigh_data.difName =
                                        rina::ApplicationProcessNamingInformation
                                        (neigh[j]
                                         .get("difName", string())
                                         .asString(),
                                         string());

                                neigh_data.supportingDifName =
                                        rina::ApplicationProcessNamingInformation
                                        (neigh[j]
                                         .get("supportingDifName",
                                              string())
                                         .asString(),
                                         string());

                                ipc.neighbors.push_back(neigh_data);

                        }
                }

                // DIFs to register at
                Json::Value difs_reg = ipc_processes[i]["difsToRegisterAt"];
                if (difs_reg != 0) {
                        for (unsigned int j = 0; j < difs_reg.size(); j++) {
                                ipc.difsToRegisterAt.push_back
                                        (rina::ApplicationProcessNamingInformation
                                         (difs_reg[j].asString(),
                                          string()));
                        }
                }

                ipc.hostname = ipc_processes[i].get
                        ("hostName", string()).asString();

                // parameters
                Json::Value params = ipc_processes[i]["parameters"];
                if (params != 0) {
                        Json::Value::Members members = params.getMemberNames();
                        for (unsigned int j = 0; j < members.size(); j++) {
                                string value = params
                                        .get(members[i], string()).asString();
                                ipc.parameters
                                        .insert(pair<string, string>
                                                (members[i], value));
                        }
                }

                ipcProcessesToCreate.push_back(ipc);
        }
}

void parse_app_to_dif(const Json::Value & root,
                      std::map<std::string,
                      rina::ApplicationProcessNamingInformation>
                      & applicationToDIFMappings)
{
        Json::Value appToDIF = root["applicationToDIFMappings"];
        if (appToDIF != 0) {
                for (unsigned int i = 0; i < appToDIF.size(); i++) {
                        string encodedAppName = appToDIF[i]
                                .get("encodedAppName", string())
                                .asString();

                        rina::ApplicationProcessNamingInformation difName =
                                rina::ApplicationProcessNamingInformation
                                (appToDIF[i]
                                 .get("difName", string())
                                 .asString(), string());

                        applicationToDIFMappings.insert
                                (pair<string,
                                 rina::ApplicationProcessNamingInformation>
                                 (encodedAppName, difName));
                }
        }
}

void parse_local_conf(const Json::Value &         root,
                      rinad::LocalConfiguration & local)
{
        Json::Value local_conf = root["localConfiguration"];
	Json::Value plugins_paths;

	if (local_conf == 0) {
		return;
	}

	local.consolePort = local_conf.get("consolePort",
					   local.consolePort).asInt();
	if (local.consolePort == 0) {
		local.consolePort = 32766;
	}

	local.installationPath = local_conf.get("installationPath",
				 local.installationPath).asString();
	if (local.installationPath.empty()) {
		local.installationPath = std::string(DEFAULT_BINDIR);
	}

	local.libraryPath = local_conf.get("libraryPath",
					   local.libraryPath).asString();
	if (local.libraryPath.empty()) {
		local.libraryPath = std::string(DEFAULT_LIBDIR);
	}

	local.logPath = local_conf.get("logPath", local.logPath).asString();
	if (local.logPath.empty()) {
		local.logPath = std::string(DEFAULT_LOGDIR);
	}

	plugins_paths = local_conf["pluginsPaths"];
	if (plugins_paths != 0) {
		for (unsigned int j = 0; j < plugins_paths.size();
				j++) {
			local.pluginsPaths.push_back(
					plugins_paths[j].asString());
		}
	}
}

void parse_dif_configs(const Json::Value   & root,
                       list<DIFTemplateMapping> & difConfigurations)
{
        Json::Value dif_configs = root["difConfigurations"];
        if (dif_configs != 0) {
        	for (unsigned int i = 0; i < dif_configs.size(); i++) {
        		rinad::DIFTemplateMapping mapping;

                        mapping.dif_name =
                                rina::ApplicationProcessNamingInformation
                                (dif_configs[i].get("name", string()).asString(),
                                 string());

                        mapping.template_name = dif_configs[i].get("template", string())
                                .asString();

                        difConfigurations.push_back(mapping);
        	}
        }
}

bool parse_configuration(std::string& file_loc)
{
        // General note: Params should be checked before they are used
        // Some can be NULL

        // Parse config file with jsoncpp
        Json::Value  root;
        Json::Reader reader;
        ifstream     file;

        file.open(file_loc.c_str());
        if (file.fail()) {
                LOG_ERR("Failed to open config file");
                return false;
        }

        if (!reader.parse(file, root, false)) {
                LOG_ERR("Failed to parse configuration");

                // FIXME: Log messages need to take string for this to work
                cout << "Failed to parse JSON" << endl
                     << reader.getFormatedErrorMessages() << endl;

                return false;
        }

        file.close();

        // Get everything in our data structures
        rinad::RINAConfiguration config;

	config.configuration_file = file_loc;
        parse_local_conf(root, config.local);
        parse_app_to_dif(root, config.applicationToDIFMappings);
        parse_ipc_to_create(root, config.ipcProcessesToCreate);
        parse_dif_configs(root, config.difConfigurations);
        IPCManager->loadConfig(config);

        return true;
}

void parse_auth_sduprot_profile(const Json::Value  & root,
                  	        rina::AuthSDUProtectionProfile & profile)
{
	parse_policy(root, "authPolicy", profile.authPolicy);
	parse_policy(root, "encryptPolicy", profile.encryptPolicy);
	parse_policy(root, "TTLPolicy", profile.ttlPolicy);
	parse_policy(root, "ErrorCheckPolicy", profile.crcPolicy);
}

rinad::DIFTemplate * parse_dif_template_config(const Json::Value & root,
					       rinad::DIFTemplate * dif_template)
{
	dif_template->difType = root.get("difType", string())
                        		.asString();

	// Data transfer constants
	Json::Value dt_const = root["dataTransferConstants"];
	if (dt_const != 0) {
		rina::DataTransferConstants dt;

		// There is no asShort()
		dt.address_length_ =
				static_cast<unsigned short>
		(dt_const
				.get("addressLength", 0)
				.asUInt());
		dt.cep_id_length_ = static_cast<unsigned short>
		(dt_const
				.get("cepIdLength", 0)
				.asUInt());
		dt.dif_integrity_ = dt_const
				.get("difIntegrity", false)
				.asBool();
		dt.length_length_ = static_cast<unsigned short>
		(dt_const
				.get("lengthLength", 0)
				.asUInt());
		dt.max_pdu_lifetime_ = dt_const
				.get("maxPduLifetime", 0)
				.asUInt();
		dt.max_pdu_size_ = dt_const
				.get("maxPduSize", 0)
				.asUInt();
		dt.port_id_length_ =
				static_cast<unsigned short>
		(dt_const
				.get("portIdLength", 0)
				.asUInt());
		dt.qos_id_length_ = static_cast<unsigned short>
		(dt_const
				.get("qosIdLength", 0)
				.asUInt());
		dt.sequence_number_length_ =
				static_cast<unsigned short>
		(dt_const
				.get("sequenceNumberLength", 0)
				.asUInt());
		dif_template->dataTransferConstants = dt;
	}

	// QoS cubes
	Json::Value cubes = root["qosCubes"];
	if (cubes != 0) {
		for (unsigned int j = 0; j < cubes.size(); j++) {
			// FIXME: Probably should have good default
			//        values. Check default constructor
			rina::QoSCube cube;

			cube.id_   = cubes[j].get("id", 0).asUInt();
			cube.name_ = cubes[j].get("name", string()).asString();

			parse_efcp_policies(cubes[j], cube);

			cube.average_bandwidth_ =
					cubes[j].get("averageBandwidth",
							cube.average_bandwidth_)
							.asUInt();
			cube.average_sdu_bandwidth_ =
					cubes[j].get("averageSduBandwidth",
							cube.average_sdu_bandwidth_)
							.asUInt();
			cube.peak_bandwidth_duration_ =
					cubes[j].get("peakBandwidthDuration",
							cube.peak_bandwidth_duration_)
							.asUInt();
			cube.peak_sdu_bandwidth_duration_ =
					cubes[j].get("peakSduBandwidthDuration",
							cube.peak_sdu_bandwidth_duration_)
							.asUInt();
			cube.undetected_bit_error_rate_ =
					cubes[j].get("undetectedBitErrorRate",
							cube.undetected_bit_error_rate_)
							.asDouble();
			cube.partial_delivery_ =
					cubes[j].get("partialDelivery",
							cube.partial_delivery_)
							.asBool();
			cube.ordered_delivery_ =
					cubes[j].get("orderedDelivery",
							cube.ordered_delivery_)
							.asBool();
			cube.max_allowable_gap_ =
					cubes[j].get("maxAllowableGap",
							cube.max_allowable_gap_)
							.asInt();
			cube.delay_ = cubes[j].get("delay", cube.delay_).asUInt();
			cube.jitter_ = cubes[j].get("jitter", cube.jitter_).asUInt();

			dif_template->qosCubes.push_back(cube);
		}
	}

	// rmtConfiguration;
	Json::Value rmt_conf = root["rmtConfiguration"];
	if (rmt_conf != 0) {
		rina::RMTConfiguration rc;

		parse_policy(rmt_conf,
			     "policySet",
			     rc.policy_set_);

	        Json::Value pft_conf = rmt_conf["pftConfiguration"];
                if (pft_conf != 0) {
                        rina::PFTConfiguration pftc;

                        parse_policy(pft_conf,
                                     "policySet",
                                     pftc.policy_set_);

                        rc.pft_conf_ = pftc;
                }
		dif_template->rmtConfiguration = rc;
	}

	// flowAllocatorConfiguration;
	Json::Value fa_conf = root["flowAllocatorConfiguration"];
	if (fa_conf != 0) {
		rina::FlowAllocatorConfiguration fac;

		parse_policy(fa_conf,
			     "policySet",
			     fac.policy_set_);

		dif_template->faConfiguration = fac;
	}

	// routingConfiguration;
	Json::Value routing_conf = root["routingConfiguration"];
	if (routing_conf != 0) {
		rina::RoutingConfiguration rc;

		parse_policy(routing_conf,
			     "policySet",
			     rc.policy_set_);

		dif_template->routingConfiguration = rc;
	}

	// resourceAllocatorConfiguration;
	Json::Value ra_conf = root["resourceAllocatorConfiguration"];
	if (ra_conf != 0) {
		rina::ResourceAllocatorConfiguration rac;

	        Json::Value pduftg_conf = ra_conf["pduftgConfiguration"];
                if (pduftg_conf != 0) {
                        rina::PDUFTGConfiguration pftgc;

		        parse_policy(pduftg_conf,
			             "policySet",
			             pftgc.policy_set_);
		        rac.pduftg_conf_ = pftgc;
                }

		dif_template->raConfiguration = rac;
	}

	// namespaceMangerConfiguration;
	Json::Value nsm_conf = root["namespaceManagerConfiguration"];
	if (nsm_conf != 0) {
		rina::NamespaceManagerConfiguration nsmc;

		parse_policy(nsm_conf,
			     "policySet",
			     nsmc.policy_set_);

		dif_template->nsmConfiguration = nsmc;
	}

	// std::list<KnownIPCProcessAddress>
	// knownIPCProcessAddresses;
	Json::Value known = root["knownIPCProcessAddresses"];
	if (known != 0) {
		for (unsigned int j = 0;
				j < known.size();
				j++) {
			rinad::KnownIPCProcessAddress kn;

			parse_name(known[j], kn.name);

			kn.address = known[j]
			                   .get("address", kn.address)
			                   .asUInt();

			dif_template->knownIPCProcessAddresses.push_back(kn);
		}
	}

	// std::list<AddressPrefixConfiguration> addressPrefixes;
	Json::Value addrp = root["addressPrefixes"];
	if (addrp != 0) {
		for (unsigned int j = 0;
				j < addrp.size();
				j++) {
			AddressPrefixConfiguration apc;

			apc.addressPrefix =
					addrp[j].get("addressPrefix",
							apc.addressPrefix)
							.asUInt();

			apc.organization =
					addrp[j].get("organization",
							string())
							.asString();

			dif_template->addressPrefixes.push_back(apc);
		}
	}

	// rina::EnrollmentTaskConfiguration
	// enrollmentTaskConfiguration;
	Json::Value etc = root["enrollmentTaskConfiguration"];
	if (etc != 0) {
		rina::EnrollmentTaskConfiguration et_conf;

		parse_policy(etc,
			     "policySet",
			     et_conf.policy_set_);

		dif_template->etConfiguration = et_conf;
	}

        //sduProtectionConfiguration
        Json::Value secManConf = root["securityManagerConfiguration"];
        if (secManConf != 0){
        	rina::SecurityManagerConfiguration sm_conf;

        	if (secManConf["policySet"] != 0) {
        		parse_policy(secManConf,
        			     "policySet",
        			     sm_conf.policy_set_);
        	}

        	Json::Value profiles = secManConf["authSDUProtProfiles"];
        	if (profiles != 0) {
        		Json::Value defaultProfile = profiles["default"];
        		if (defaultProfile != 0) {
        			parse_auth_sduprot_profile(defaultProfile,
        						   sm_conf.default_auth_profile);
        		}

        		Json::Value specifics = profiles["specific"];
        		if (specifics != 0) {
        			for (unsigned int j = 0;
        					j < specifics.size();
        					j++) {
        				rina::AuthSDUProtectionProfile profile;
        				std::string dif_name;

        				parse_auth_sduprot_profile(specifics[j], profile);

        				dif_name = specifics[j]
        				                   .get("underlyingDIF", dif_name)
        				                   .asString();

        				sm_conf.specific_auth_profiles[dif_name] = profile;
        			}
        		}
        	}

        	dif_template->secManConfiguration = sm_conf;
        }

	// configParameters;
	Json::Value confParams = root["configParameters"];

	if (confParams != 0) {
		Json::Value::Members members =
				confParams.getMemberNames();
		for (unsigned int j = 0; j < members.size(); j++) {
			string value = confParams
					.get(members[j], string()).asString();
			dif_template->configParameters
			.insert(pair<string, string>
			(members[j], value));
		}
	}

	return dif_template;
}

DIFTemplate * parse_dif_template(const std::string& file_name,
				 const std::string& template_name)
{
        // Parse config file with jsoncpp
        Json::Value  root;
        Json::Reader reader;
        ifstream     file;

        file.open(file_name.c_str());
        if (file.fail()) {
                LOG_ERR("Failed to open config file");
                return 0;
        }

        if (!reader.parse(file, root, false)) {
        	LOG_ERR("Failed to parse configuration");

        	// FIXME: Log messages need to take string for this to work
        	cout << "Failed to parse JSON" << endl
        			<< reader.getFormatedErrorMessages() << endl;

        	return 0;
        }

        file.close();

        DIFTemplate * dif_template = new DIFTemplate();
        dif_template->templateName = template_name;
        parse_dif_template_config(root, dif_template);

        return dif_template;
}

}
