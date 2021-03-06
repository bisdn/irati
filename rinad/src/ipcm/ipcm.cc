/*
 * IPC Manager
 *
 *    Vincenzo Maffione     <v.maffione@nextworks.it>
 *    Francesco Salvestrini <f.salvestrini@nextworks.it>
 *    Eduard Grasa          <eduard.grasa@i2cat.net>
 *    Marc Sune             <marc.sune (at) bisdn.de>
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
#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

#include <librina/common.h>
#include <librina/ipc-manager.h>
#include <librina/plugin-info.h>

#define RINA_PREFIX "ipcm"
#include <librina/logs.h>
#include <debug.h>

#include "rina-configuration.h"
#include "ipcm.h"
#include "dif-validator.h"

//Addons
#include "addons/console.h"
#include "addons/scripting.h"
//[+] Add more here...

//IPCM IPCP
#include "ipcp.h"

//Handler specifics (transaction states)
#include "ipcp-handlers.h"
#include "flow-alloc-handlers.h"
#include "misc-handlers.h"

//Timeouts for timed wait
#define IPCM_EVENT_TIMEOUT_S 0
#define IPCM_EVENT_TIMEOUT_NS 100000000 //0.1 sec
#define IPCM_TRANS_TIMEOUT_S 7

//Downcast MACRO
#ifndef DOWNCAST_DECL
	// Useful MACRO to perform downcasts in declarations.
	#define DOWNCAST_DECL(_var,_class,_name)\
		_class *_name = dynamic_cast<_class*>(_var);
#endif //DOWNCAST_DECL


using namespace std;

namespace rinad {

//
//IPCManager_
//

//Singleton instance
Singleton<IPCManager_> IPCManager;

IPCManager_::IPCManager_() : req_to_stop(false), io_thread(NULL),
		dif_template_manager(NULL) {

}

IPCManager_::~IPCManager_()
{
	if (dif_template_manager) {
		delete dif_template_manager;
	}
}

void IPCManager_::init(const std::string& loglevel, std::string& config_file)
{
	// Initialize the IPC manager infrastructure in librina.

	try {
		rina::initializeIPCManager(1,
					   config.local.installationPath,
					   config.local.libraryPath,
					   loglevel,
					   config.local.logPath);
		LOG_DBG("IPC Manager daemon initialized");
		LOG_DBG("       installation path: %s",
			config.local.installationPath.c_str());
		LOG_DBG("       library path: %s",
			config.local.libraryPath.c_str());
		LOG_DBG("       log folder: %s", config.local.logPath.c_str());

		// Load the plugins catalog
		catalog.import();
		catalog.print();

		// Initialize the I/O thread
		io_thread = new rina::Thread(io_loop_trampoline,
				             NULL,
				             &io_thread_attrs);
		io_thread->start();

		// Initialize DIF Templates Manager (with its monitor thread)
		stringstream ss;
		ss << config_file.substr(0, config_file.rfind("/"));
		dif_template_manager = new DIFTemplateManager(ss.str());
	} catch (rina::InitializationException& e) {
		LOG_ERR("Error while initializing librina-ipc-manager");
		exit(EXIT_FAILURE);
	}
}

void
IPCManager_::load_addons(const std::string& addon_list){

	std::string al = addon_list;

	//Convert the list of addons to lowercase
	std::transform(al.begin(), al.end(), al.begin(), ::tolower);

	//Split comma based, and remove extra chars (spaces)
	std::stringstream ss(al);
	std::string t;
	while(std::getline(ss, t, ',')) {
		//Remove whitespaces
		t.erase(std::remove_if( t.begin(), t.end(), ::isspace ),
								t.end() );

		//Call the factory
		Addon::factory(config, t);
	}
}

/*
*
* Public API
*
*/

ipcm_res_t
IPCManager_::create_ipcp(Addon* callee, CreateIPCPPromise* promise,
			const rina::ApplicationProcessNamingInformation& name,
			const std::string& type)
{
	IPCMIPCProcess *ipcp;
	ostringstream ss;
	rina::IPCProcessFactory fact;
	std::list<std::string> ipcp_types;
	bool difCorrect = false;
	std::string s;
	SyscallTransState* trans;
	int ipcp_id = -1;

	try {
		// Check that the AP name is not empty
		if (name.processName == std::string("")) {
			ss << "Cannot create IPC process with an "
				"empty AP name" << endl;
			FLUSH_LOG(ERR, ss);
			throw rina::CreateIPCProcessException();
		}

		//Check if dif type exists
		list_ipcp_types(ipcp_types);
		if(std::find(ipcp_types.begin(), ipcp_types.end(), type)
							== ipcp_types.end()){
			ss << "IPCP type parameter "
				   << name.toString()
				   << " is wrong, options are: "
				   << s;
				FLUSH_LOG(ERR, ss);
				throw rina::CreateIPCProcessException();
		}

		//Call the factory
		ipcp = ipcp_factory_.create(name, type);

		//Get the ipcp_id and reduce locking scope
		ipcp_id = ipcp->get_id();

		//Set the promise
		if (promise)
			promise->ipcp_id = ipcp_id;

		//TODO: this should be moved to the factory
		//Moreover the API should be homgenized such that the
		if (type != rina::NORMAL_IPC_PROCESS) {
			// Shim IPC processes are set as initialized
			// immediately.
			ipcp->setInitialized();

			//Release the lock asap
			ipcp->rwlock.unlock();

			//And mark the promise as completed
			if (promise) {
				promise->ret = IPCM_SUCCESS;
				promise->signal();
			}

			//Show a nice trace
			ss << "IPC process " << name.toString() << " created "
				"[id = " << ipcp_id << "]" << endl;
			FLUSH_LOG(INFO, ss);

			//Distribute the event to the addons
			IPCMEvent addon_e(callee, IPCM_IPCP_CREATED,
							ipcp_id);
			Addon::distribute_ipcm_event(addon_e);

			return IPCM_SUCCESS;
		} else {

			//Release the lock asap
			ipcp->rwlock.unlock();

			// Normal IPC processes can be set as
			// initialized only when the corresponding
			// IPC process daemon is initialized, so we
			// defer the operation.

			//Add transaction state
			trans = new SyscallTransState(callee, promise,
							ipcp_id);
			if(!trans){
				assert(0);
				ss << "Failed to create IPC process '" <<
							name.toString() << "' of type '" <<
							type << "'. Out of memory!" << endl;
						FLUSH_LOG(ERR, ss);
				FLUSH_LOG(ERR, ss);
				throw rina::CreateIPCProcessException();
			}

			if(add_syscall_transaction_state(trans) < 0){
				assert(0);
				throw rina::CreateIPCProcessException();
			}
			//Show a nice trace
			ss << "IPC process " << name.toString() << " created and waiting for initialization"
				"[id = " << ipcp_id << "]" << endl;
			FLUSH_LOG(INFO, ss);

		}
	} catch(rina::ConcurrentException& e) {
		ss << "Failed to create IPC process '" <<
			name.toString() << "' of type '" <<
			type << "'. Transaction timed out" << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch(...) {
		ss << "Failed to create IPC process '" <<
			name.toString() << "' of type '" <<
			type << "'" << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}


	return IPCM_PENDING;
}

ipcm_res_t
IPCManager_::destroy_ipcp(Addon* callee, unsigned short ipcp_id)
{
	ostringstream ss;

	//Distribute the event to the addons
	IPCMEvent addon_e(callee, IPCM_IPCP_TO_BE_DESTROYED,
					ipcp_id);
	Addon::distribute_ipcm_event(addon_e);

	try {
		ipcp_factory_.destroy(ipcp_id);
		ss << "IPC process destroyed [id = " << ipcp_id
			<< "]" << endl;
		FLUSH_LOG(INFO, ss);
	} catch (rina::DestroyIPCProcessException& e) {
		ss  << ": Error while destroying IPC "
			"process with id " << ipcp_id << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	// Synchronize the catalog state, so that
	// the ipcp_id of the IPCP just destroyed can
	// be reused without inconsistencies in the catalog
	catalog.ipcp_destroyed(ipcp_id);

	return IPCM_SUCCESS;
}

void
IPCManager_::list_ipcps(std::ostream& os)
{
	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

	vector<IPCMIPCProcess *> ipcps;
	ipcp_factory_.listIPCProcesses(ipcps);

	os << "Current IPC processes (id | name | type | state | Registered applications | Port-ids of flows provided)" << endl;
	for (unsigned int i = 0; i < ipcps.size(); i++) {
		ipcps[i]->get_description(os);
	}
}


//NOTE: this assumes an empty name is invalid as a return value for
//could not complete the operation
std::string
IPCManager_::get_ipcp_name(int ipcp_id)
{
	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

	IPCMIPCProcess* ipcp = lookup_ipcp_by_id(ipcp_id, false);

	if(!ipcp)
		return "";

	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock rreadlock(ipcp->rwlock, false);

	if(!ipcp)
		return std::string("");

	return ipcp->get_name().processName;
}


void
IPCManager_::list_ipcps(std::list<int>& list)
{
	//Prevent any insertion/deletion to happen
	rina::ReadScopedLock readlock(ipcp_factory_.rwlock);

	//Call the factory
	std::vector<IPCMIPCProcess*> ipcps;
	ipcp_factory_.listIPCProcesses(ipcps);

	//Compose the list
	std::vector<IPCMIPCProcess*>::const_iterator it;
	for(it = ipcps.begin(); it != ipcps.end(); ++it)
		list.push_back((*it)->get_id());
}

bool
IPCManager_::ipcp_exists(const unsigned short ipcp_id){
	return ipcp_factory_.exists(ipcp_id);
}

void
IPCManager_::list_ipcp_types(std::list<std::string>& types)
{
	types = ipcp_factory_.getSupportedIPCProcessTypes();
}

//TODO this assumes single IPCP per DIF
int IPCManager_::get_ipcp_by_dif_name(std::string& difName){

	IPCMIPCProcess* ipcp;
	int ret;
	rina::ApplicationProcessNamingInformation dif(difName, string());

	ipcp = select_ipcp_by_dif(dif);
	if(!ipcp)
		ret = -1;
	else
		ret = ipcp->get_id();

	return ret;
}

ipcm_res_t
IPCManager_::assign_to_dif(Addon* callee, Promise* promise,
			const unsigned short ipcp_id,
			rinad::DIFTemplate * dif_template,
			const rina::ApplicationProcessNamingInformation &
								  dif_name)
{
	rina::DIFInformation dif_info;
	rina::DIFConfiguration dif_config;
	ostringstream ss;
	IPCMIPCProcess* ipcp;
	IPCPTransState* trans;

	try {
		if (is_any_ipcp_assigned_to_dif(dif_name)) {
			ss << "There is already an IPCP assigned to DIF "
				<< dif_name.toString()
				<< " in this system.";
			FLUSH_LOG(ERR, ss);
			throw rina::AssignToDIFException();
		}

		ipcp = lookup_ipcp_by_id(ipcp_id, false);
		if (ipcp->get_type() == rina::NORMAL_IPC_PROCESS) {
			// Load all the plugins required from by template, but
			// first release the ipcp lock, to avoid lock-ups.
			ipcp->rwlock.unlock();

			catalog.load_by_template(callee, ipcp_id, dif_template);

		} else {
			ipcp->rwlock.unlock();
		}

		ipcp = lookup_ipcp_by_id(ipcp_id, true);
		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
			FLUSH_LOG(ERR, ss);
			throw rina::AssignToDIFException();
		}

		//Auto release the write lock
		rina::WriteScopedLock writelock(ipcp->rwlock, false);

		// Fill in the DIFConfiguration object.
		if (ipcp->get_type() == rina::NORMAL_IPC_PROCESS) {
			rina::EFCPConfiguration efcp_config;
			rina::NamespaceManagerConfiguration nsm_config;
			rina::AddressingConfiguration address_config;
			unsigned int address;

			// FIll in the EFCPConfiguration object.
			efcp_config.set_data_transfer_constants(
					dif_template->dataTransferConstants);
			rina::QoSCube * qosCube = 0;
			for (list<rina::QoSCube>::iterator
					qit = dif_template->qosCubes.begin();
					qit != dif_template->qosCubes.end();
					qit++) {
				qosCube = new rina::QoSCube(*qit);
				if(!qosCube){
					ss << "Unable to allocate memory for the QoSCube object. Out of memory! "
					<< dif_name.toString();
					FLUSH_LOG(ERR, ss);
					throw rina::Exception();
				}
				efcp_config.add_qos_cube(qosCube);
			}

			for (list<AddressPrefixConfiguration>::iterator
					ait = dif_template->addressPrefixes.begin();
					ait != dif_template->addressPrefixes.end();
					ait ++) {
				rina::AddressPrefixConfiguration prefix;
				prefix.address_prefix_ = ait->addressPrefix;
				prefix.organization_ = ait->organization;
				address_config.address_prefixes_.push_back(prefix);
			}

			for (list<rinad::KnownIPCProcessAddress>::iterator
					kit = dif_template->knownIPCProcessAddresses.begin();
					kit != dif_template->knownIPCProcessAddresses.end();
					kit ++) {
				rina::StaticIPCProcessAddress static_address;
				static_address.ap_name_ = kit->name.processName;
				static_address.ap_instance_ = kit->name.processInstance;
				static_address.address_ = kit->address;
				address_config.static_address_.push_back(static_address);
			}
			nsm_config.addressing_configuration_ = address_config;
			nsm_config.policy_set_ = dif_template->nsmConfiguration.policy_set_;

			bool found = dif_template->
				lookup_ipcp_address(ipcp->get_name(),
						address);
			if (!found) {
				ss << "No address for IPC process " <<
					ipcp->get_name().toString() <<
					" in DIF " << dif_name.toString() <<
					endl;
				FLUSH_LOG(ERR, ss);
				throw rina::Exception();
			}
			dif_config.efcp_configuration_ = efcp_config;
			dif_config.nsm_configuration_ = nsm_config;
			dif_config.rmt_configuration_ = dif_template->rmtConfiguration;
			dif_config.fa_configuration_ = dif_template->faConfiguration;
			dif_config.ra_configuration_ = dif_template->raConfiguration;
			dif_config.routing_configuration_ = dif_template->routingConfiguration;
			dif_config.sm_configuration_ = dif_template->secManConfiguration;
			dif_config.et_configuration_ = dif_template->etConfiguration;
			dif_config.set_address(address);

			dif_config.sm_configuration_ = dif_template->secManConfiguration;
		}

		for (map<string, string>::const_iterator
				pit = dif_template->configParameters.begin();
				pit != dif_template->configParameters.end();
				pit++) {
			dif_config.add_parameter
				(rina::Parameter(pit->first, pit->second));
		}

		// Fill in the DIFInformation object.
		dif_info.set_dif_name(dif_name);
		dif_info.set_dif_type(ipcp->get_type());
		dif_info.set_dif_configuration(dif_config);

		// Validate the parameters
		DIFConfigValidator validator(dif_config, dif_info,
				ipcp->get_type());
		if(!validator.validateConfigs())
			throw rina::BadConfigurationException("DIF configuration validator failed");

		//Create a transaction
		trans = new IPCPTransState(callee, promise, ipcp->get_id());
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! "
				<< dif_name.toString();
			FLUSH_LOG(ERR, ss);
			throw rina::AssignToDIFException();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? "
				<< dif_name.toString();
			FLUSH_LOG(ERR, ss);
			throw rina::AssignToDIFException();
		}

		ipcp->assignToDIF(dif_info, trans->tid);

		ss << "Requested DIF assignment of IPC process " <<
			ipcp->get_name().toString() << " to DIF " <<
			dif_name.toString() << endl;
		FLUSH_LOG(INFO, ss);
	} catch(rina::ConcurrentException& e) {
		ss << "Error while assigning " <<
			ipcp->get_name().toString() <<
			" to DIF " << dif_name.toString() <<
			". Operation timedout"<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::AssignToDIFException& e) {
		ss << "Error while assigning " <<
			ipcp->get_name().toString() <<
			" to DIF " << dif_name.toString() << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::BadConfigurationException& e) {
		LOG_ERR("Asssign IPCP %d to dif %s failed. Bad configuration.",
						ipcp_id,
						dif_name.toString().c_str());
		return IPCM_FAILURE;
	}catch (rina::Exception &e) {
		LOG_ERR("Asssign IPCP %d to dif %s failed. Unknown error catched: %s:%d",
						ipcp_id,
						dif_name.toString().c_str(),
						__FILE__,
						__LINE__);
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

ipcm_res_t
IPCManager_::register_at_dif(Addon* callee, Promise* promise,
			const unsigned short ipcp_id,
			const rina::ApplicationProcessNamingInformation& dif_name,
			bool blocking)
{
	// Select a slave (N-1) IPC process.
	IPCMIPCProcess *ipcp, *slave_ipcp;
	ostringstream ss;
	IPCPregTransState* trans;

	// Try to register @ipcp to the slave IPC process.
	try {
		ipcp = lookup_ipcp_by_id(ipcp_id, true);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		rina::WriteScopedLock writelock(ipcp->rwlock, false);
		slave_ipcp = select_ipcp_by_dif(dif_name, true);

		if (!slave_ipcp) {
			ss << "Cannot find any IPC process belonging "
			   << "to DIF " << dif_name.toString()
			   << endl;
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		rina::WriteScopedLock swritelock(slave_ipcp->rwlock, false);

		//Create a transaction
		trans = new IPCPregTransState(callee, promise, ipcp->get_id(),
							slave_ipcp->get_id());
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! "
				<< dif_name.toString();
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? "
				<< dif_name.toString();
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Register
		slave_ipcp->registerApplication(
				ipcp->get_name(), ipcp->get_id(), trans->tid, blocking);

		ss << "Requested DIF registration of IPC process " <<
			ipcp->get_name().toString() << " at DIF " <<
			dif_name.toString() << " through IPC process "
		   << slave_ipcp->get_name().toString()
		   << endl;
		FLUSH_LOG(INFO, ss);
	} catch(rina::ConcurrentException& e) {
		ss  << ": Error while requesting registration. Operation timedout" << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::Exception& e) {
		ss  << ": Error while requesting registration: "
		    << e.what() << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

ipcm_res_t
IPCManager_::unregister_ipcp_from_ipcp(Addon* callee, Promise* promise,
		const unsigned short ipcp_id,
		const rina::ApplicationProcessNamingInformation& dif_name)
{
	ostringstream ss;
	IPCMIPCProcess *ipcp, *slave_ipcp;
	IPCPregTransState* trans;

	try {

		ipcp = lookup_ipcp_by_id(ipcp_id, true);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		rina::WriteScopedLock writelock(ipcp->rwlock, false);
		slave_ipcp = select_ipcp_by_dif(dif_name, true);

		if (!slave_ipcp) {
			ss << "Cannot find any IPC process belonging "
			   << "to DIF " << dif_name.toString()
			   << endl;
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		rina::WriteScopedLock swritelock(slave_ipcp->rwlock, false);

		//Create a transaction
		trans = new IPCPregTransState(callee, promise, ipcp->get_id(),
				slave_ipcp->get_id());

		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		// Forward the unregistration request to the IPC process
		// that the client IPC process is registered to
		slave_ipcp->unregisterApplication(ipcp->get_name(),
				trans->tid);

		ss << "Requested unregistration of IPC process " <<
				ipcp->get_name().toString() << " from IPC "
				"process " << slave_ipcp->get_name().toString() << endl;
		FLUSH_LOG(INFO, ss);
	} catch(rina::ConcurrentException& e) {
		ss  << ": Error while unregistering IPC process "
				<< ipcp->get_name().toString() << " from IPC "
				"process " << slave_ipcp->get_name().toString() <<
				"The operation timedout"<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::IpcmUnregisterApplicationException& e) {
		ss  << ": Error while unregistering IPC process "
				<< ipcp->get_name().toString() << " from IPC "
				"process " << slave_ipcp->get_name().toString() << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::Exception& e) {
		ss  << ": Unknown error while requesting unregistering IPCP"
				<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

ipcm_res_t
IPCManager_::enroll_to_dif(Addon* callee, Promise* promise, const unsigned short ipcp_id,
			  const rinad::NeighborData& neighbor)
{
	ostringstream ss;
	IPCMIPCProcess *ipcp;
	IPCPTransState* trans;

	try {
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
                	FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Auto release the write lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);

		//Create a transaction
		trans = new IPCPTransState(callee, promise, ipcp->get_id());

		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! "
				<< neighbor.difName.toString();
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? "
				<< neighbor.difName.toString();
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		ipcp->enroll(neighbor.difName,
				neighbor.supportingDifName,
				neighbor.apName, trans->tid);

		ss << "Requested enrollment of IPC process " <<
			ipcp->get_name().toString() << " to DIF " <<
			neighbor.difName.toString() << " through DIF "
			<< neighbor.supportingDifName.toString() <<
			" and neighbor IPC process " <<
			neighbor.apName.toString() << endl;
		FLUSH_LOG(INFO, ss);
	} catch(rina::ConcurrentException& e) {
		ss  << ": Error while enrolling "
			<< "to DIF " << neighbor.difName.toString()
			<<". Operation timedout"<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}  catch (rina::EnrollException& e) {
		ss  << ": Error while enrolling "
			<< "to DIF " << neighbor.difName.toString()
			<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::Exception& e) {
		ss  << ": Unknown error while enrolling IPCP"
		    << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

bool IPCManager_::lookup_dif_by_application(
	const rina::ApplicationProcessNamingInformation& apName,
	rina::ApplicationProcessNamingInformation& difName){
	return config.lookup_dif_by_application(apName, difName);
}

ipcm_res_t
IPCManager_::apply_configuration()
{
	ostringstream ss;
	list<int> ipcps;
	list<rinad::IPCProcessToCreate>::iterator cit;
	list<int>::iterator pit;

	//TODO: move this to a write_lock over the IPCP

	try{
		//FIXME TODO XXX this method needs to be heavily refactored
		//It is not clear which exceptions can be thrown and what to do
		//if this happens. Just protecting to prevent dead-locks.

		// Examine all the IPCProcesses that are going to be created
		// according to the configuration file.
		ipcm_res_t result;
		CreateIPCPPromise c_promise;
		Promise promise;
		bool found;
		rinad::DIFTemplateMapping template_mapping;
		rinad::DIFTemplate * dif_template;
		for (cit = config.ipcProcessesToCreate.begin();
		     cit != config.ipcProcessesToCreate.end(); cit++) {
			ostringstream      ss;

			found = config.lookup_dif_template_mappings(cit->difName, template_mapping);
			if (!found) {
				ss << "Could not find DIF template for dif name "
						<< cit->difName.processName <<endl;
				FLUSH_LOG(ERR, ss);
				continue;
			}

			dif_template = dif_template_manager->get_dif_template(template_mapping.template_name);
			if (!dif_template) {
				ss << "Cannot find template called " << template_mapping.template_name;
				FLUSH_LOG(ERR, ss);
				continue;
			}

			try {
				if (create_ipcp(NULL, &c_promise, cit->name, dif_template->difType) == IPCM_FAILURE ||
						c_promise.wait() != IPCM_SUCCESS) {
					continue;
				}
				ipcps.push_back(c_promise.ipcp_id);

				if (assign_to_dif(NULL, &promise, c_promise.ipcp_id, dif_template,
						cit->difName) == IPCM_FAILURE || promise.wait() != IPCM_SUCCESS) {
					ss << "Problems assigning IPCP " << c_promise.ipcp_id
							<< " to DIF " << cit->difName.processName <<endl;
					FLUSH_LOG(ERR, ss);
				}

				for (list<rina::ApplicationProcessNamingInformation>::const_iterator
						nit = cit->difsToRegisterAt.begin();
						nit != cit->difsToRegisterAt.end(); nit++) {
					if (register_at_dif(NULL, &promise, c_promise.ipcp_id, *nit, true) == IPCM_FAILURE ||
							promise.wait() != IPCM_SUCCESS) {
						ss << "Problems registering IPCP " << c_promise.ipcp_id
								<< " to DIF " << nit->processName << endl;
						FLUSH_LOG(ERR, ss);
					}
				}
			} catch (rina::Exception &e) {
				LOG_ERR("Exception while applying configuration: %s",
					e.what());
				continue;
			}
		}

		// Perform all the enrollments specified by the configuration file.
		for (pit = ipcps.begin(), cit = config.ipcProcessesToCreate.begin();
				pit != ipcps.end();
				pit++, cit++) {
			Promise promise;
			try{
				for (list<rinad::NeighborData>::const_iterator
						nit = cit->neighbors.begin();
						nit != cit->neighbors.end(); nit++) {
					if (enroll_to_dif(NULL, &promise, *pit, *nit) == IPCM_FAILURE ||
							promise.wait() != IPCM_SUCCESS) {
						ss  << ": Unknown error while enrolling IPCP " << *pit
							<< " to neighbour " << nit->apName.getEncodedString() << endl;
						FLUSH_LOG(ERR, ss);
						continue;
					}
				}
			} catch (rina::Exception& e) {
				ss  << ": Unknown error while enrolling IPCP "<<
						*pit << " to neighbours." << endl;
				FLUSH_LOG(ERR, ss);
				continue;
			}
		}
	} catch (rina::Exception &e) {
		LOG_ERR("Exception while applying configuration: %s",
								e.what());
		return IPCM_FAILURE;
	}

	return IPCM_SUCCESS;
}

ipcm_res_t
IPCManager_::update_dif_configuration(Addon* callee, Promise* promise, const unsigned short ipcp_id,
				     const rina::DIFConfiguration & dif_config)
{
	ostringstream ss;
	IPCMIPCProcess *ipcp;
	IPCPTransState* trans;

	try {
		ipcp = lookup_ipcp_by_id(ipcp_id, true);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
                	FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Auto release the write lock
		rina::WriteScopedLock writelock(ipcp->rwlock, false);

		// Request a configuration update for the IPC process
		/* FIXME The meaning of this operation is unclear: what
		 * configuration is modified? The configuration of the
		 * IPC process only or the configuration of the whole DIF
		 * (which possibly contains more IPC process, both on the same
		 * processing systems and on different processing systems) ?
		 */
		trans = new IPCPTransState(callee, promise, ipcp->get_id());

		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}
		ipcp->updateDIFConfiguration(dif_config, trans->tid);

		ss << "Requested configuration update for IPC process " <<
			ipcp->get_name().toString() << endl;
		FLUSH_LOG(INFO, ss);
	} catch(rina::ConcurrentException& e) {
		ss  << ": Error while updating DIF configuration "
			" for IPC process " << ipcp->get_name().toString() <<
			"Operation timedout."<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}  catch (rina::UpdateDIFConfigurationException& e) {
		ss  << ": Error while updating DIF configuration "
			" for IPC process " << ipcp->get_name().toString() << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::Exception& e) {
		ss  << ": Unknown error while update configuration"
		    << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

ipcm_res_t
IPCManager_::query_rib(Addon* callee, QueryRIBPromise* promise, const unsigned short ipcp_id)
{
	ostringstream ss;
	IPCMIPCProcess *ipcp;
	RIBqTransState* trans;

	try {
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
                	FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);

		trans = new RIBqTransState(callee, promise, ipcp->get_id());
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		ipcp->queryRIB("", "", 0, 0, "", trans->tid);

		ss << "Requested query RIB of IPC process " <<
			ipcp->get_name().toString() << endl;
		FLUSH_LOG(INFO, ss);
	} catch(rina::ConcurrentException& e) {
		ss << "Error while querying RIB of IPC Process " <<
			ipcp->get_name().toString() <<
			". Operation timedout"<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::QueryRIBException& e) {
		ss << "Error while querying RIB of IPC Process " <<
			ipcp->get_name().toString() << endl;
		return IPCM_FAILURE;
	}catch (rina::Exception& e) {
		ss  << ": Unknown error while query RIB"
		    << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

std::string IPCManager_::get_log_level() const
{
	return log_level_;
}

ipcm_res_t
IPCManager_::set_policy_set_param(Addon* callee, Promise* promise,
		const unsigned short ipcp_id,
		const std::string& component_path,
		const std::string& param_name,
		const std::string& param_value)
{
	ostringstream ss;
	IPCPTransState* trans = NULL;
	IPCMIPCProcess *ipcp;

	try {
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
			FLUSH_LOG(ERR, ss);
			throw rina::SetPolicySetParamException();
		}

		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);

		trans = new IPCPTransState(callee, promise, ipcp->get_id());
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			FLUSH_LOG(ERR, ss);
			throw rina::SetPolicySetParamException();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			FLUSH_LOG(ERR, ss);
			throw rina::SetPolicySetParamException();
		}

		ipcp->setPolicySetParam(component_path,
				param_name, param_value, trans->tid);

		ss << "Issued set-policy-set-param to IPC process " <<
				ipcp->get_name().toString() << endl;
		FLUSH_LOG(INFO, ss);

	} catch(rina::ConcurrentException& e) {
		ss << "Error while issuing set-policy-set-param request "
				"to IPC Process " << ipcp->get_name().toString()
				<< ". Operation timedout"<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::SetPolicySetParamException& e) {
		ss << "Error while issuing set-policy-set-param request "
				"to IPC Process " << ipcp->get_name().toString() << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}catch (rina::Exception& e) {
		ss  << ": Unknown error while issuing set-policy-set-param request"
				<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

ipcm_res_t
IPCManager_::select_policy_set(Addon* callee, Promise* promise,
		const unsigned short ipcp_id,
		const std::string& component_path,
		const std::string& ps_name)
{
	ostringstream ss;
	IPCMIPCProcess *ipcp;
	IPCPTransState* trans;

	try {
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);

		trans = new IPCPTransState(callee, promise, ipcp->get_id());
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		ipcp->selectPolicySet(component_path, ps_name, trans->tid);

		ss << "Issued select-policy-set to IPC process " <<
				ipcp->get_name().toString() << endl;
		FLUSH_LOG(INFO, ss);
	} catch(rina::ConcurrentException& e) {
		ss << "Error while issuing select-policy-set request "
				"to IPC Process " << ipcp->get_name().toString() <<
				". Operation timedout."<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::SelectPolicySetException& e) {
		ss << "Error while issuing select-policy-set request "
				"to IPC Process " << ipcp->get_name().toString() << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}catch (rina::Exception& e) {
		ss  << ": Unknown error while issuing select-policy-set request"
				<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

// Returns IPCM_SUCCESS if a kernel plugin was successfully loaded/unloaded,
// IPCM_FAILURE otherwise
ipcm_res_t
IPCManager_::plugin_load_kernel(const std::string& plugin_name,
				bool load)
{
	ostringstream ss;
	ipcm_res_t result = IPCM_FAILURE;
	pid_t pid;

	pid = fork();
	if (pid < 0) {
		// parent, fork() failed
		ss << "Kernel plugin (un)loading: fork() failed";
		FLUSH_LOG(ERR, ss);
		result = IPCM_FAILURE;

	} else if (pid == 0) {
		// child
		if (load) {
			execlp("modprobe", "modprobe", plugin_name.c_str(),
			       NULL);

		} else {
			execlp("modprobe", "modprobe", "-r",
			       plugin_name.c_str(), NULL);
		}

		ss << "Kernel plugin (un)loading: exec() failed";
		FLUSH_LOG(ERR, ss);

		exit(EXIT_FAILURE);

	} else {
		// parent, fork() successful
		int child_status = 0;

		waitpid(pid, &child_status, 0);

		result = child_status ? IPCM_FAILURE : IPCM_SUCCESS;
	}

	return result;
}

ipcm_res_t
IPCManager_::plugin_load(Addon* callee, Promise* promise,
		const unsigned short ipcp_id,
		const std::string& plugin_name, bool load)
{
	ostringstream ss;
	IPCMIPCProcess *ipcp;
	IPCPTransState* trans;

	try {
		//First try to see if its a kernel module
		if (plugin_load_kernel(plugin_name, load) == IPCM_SUCCESS) {
			promise->ret = IPCM_SUCCESS;

			return IPCM_SUCCESS;
		}

		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			ss << "Invalid IPCP id "<< ipcp_id;
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);

		trans = new IPCPTransState(callee, promise, ipcp->get_id());
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}
		ipcp->pluginLoad(plugin_name, load, trans->tid);

		ss << "Issued plugin-load to IPC process " <<
		      ipcp->get_name().toString() << endl;
		FLUSH_LOG(INFO, ss);
	} catch(rina::ConcurrentException& e) {
		ss << "Error while issuing plugin-load request "
				"to IPC Process " << ipcp->get_name().toString() <<
				". Operation timedout"<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::PluginLoadException& e) {
		ss << "Error while issuing plugin-load request "
				"to IPC Process " << ipcp->get_name().toString() << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::Exception& e) {
		ss  << ": Unknown error while issuing plugin-load request "
				<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

ipcm_res_t
IPCManager_::plugin_get_info(const std::string& plugin_name,
			     std::list<rina::PsInfo>& result)
{
	int ret = rina::plugin_get_info(plugin_name, IPCPPLUGINSDIR, result);

	return ret ? IPCM_FAILURE : IPCM_SUCCESS;
}

ipcm_res_t
IPCManager_::read_ipcp_ribobj(Addon* callee, Promise* promise,
			      const unsigned short ipcp_id,
			      const std::string& object_class,
			      const std::string& object_name)
{
	IPCMIPCProcess * ipcp;
	TransactionState* trans;
	ostringstream ss;

	try {
		ipcp = lookup_ipcp_by_id(ipcp_id);

		if(!ipcp){
			LOG_ERR("Invalid IPCP id %hu", ipcp_id);
			return IPCM_FAILURE;
		}

		//Auto release the read lock
		rina::ReadScopedLock readlock(ipcp->rwlock, false);

		rina::CDAPMessage *msg = rina::CDAPMessage::getReadObjectRequestMessage(NULL,
				rina::CDAPMessage::NONE_FLAGS, object_class, 0, object_name, 0);

		trans = new TransactionState(callee, promise);
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		ipcp->forwardCDAPMessage(*msg, trans->tid);
		delete msg;

		ss << "Forwarded CDAPMessage to IPC process " <<
		      ipcp->get_name().toString() << endl;
		FLUSH_LOG(INFO, ss);

	} catch  (rina::Exception &e) {
		LOG_ERR("Problems: %s", e.what());
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

ipcm_res_t
IPCManager_::unregister_app_from_ipcp(Addon* callee, Promise* promise,
		const rina::ApplicationUnregistrationRequestEvent& req_event,
		const unsigned short slave_ipcp_id)
{
	ostringstream ss;
	IPCMIPCProcess *slave_ipcp;
	IPCPTransState* trans;

	try {
		slave_ipcp = lookup_ipcp_by_id(slave_ipcp_id, true);

		if (!slave_ipcp) {
			ss << "Cannot find any IPC process belonging "<<endl;
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Auto release the write lock
		rina::WriteScopedLock writelock(slave_ipcp->rwlock, false);

		// Forward the unregistration request to the IPC process
		// that the application is registered to
		trans = new IPCPTransState(callee, promise,
							slave_ipcp->get_id());
		if(!trans){
			ss << "Unable to allocate memory for the transaction object. Out of memory! ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		//Store transaction
		if(add_transaction_state(trans) < 0){
			ss << "Unable to add transaction; out of memory? ";
			FLUSH_LOG(ERR, ss);
			throw rina::Exception();
		}

		slave_ipcp->unregisterApplication(req_event.applicationName,
				trans->tid);
		ss << "Requested unregistration of application " <<
				req_event.applicationName.toString() << " from IPC "
				"process " << slave_ipcp->get_name().toString() << endl;
		FLUSH_LOG(INFO, ss);
	} catch(rina::ConcurrentException& e) {
		ss  << ": Error while unregistering application "
				<< req_event.applicationName.toString() << " from IPC "
				"process " << slave_ipcp->get_name().toString() <<
				". Operation timedout."<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::IpcmUnregisterApplicationException& e) {
		ss  << ": Error while unregistering application "
				<< req_event.applicationName.toString() << " from IPC "
				"process " << slave_ipcp->get_name().toString() << endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	} catch (rina::Exception& e) {
		ss  << ": Unknown error while unregistering application "
				<< endl;
		FLUSH_LOG(ERR, ss);
		return IPCM_FAILURE;
	}

	return IPCM_PENDING;
}

//
// Promises
//
ipcm_res_t Promise::wait(void){
	unsigned int i;
	// Due to the async nature of the API, notifications (signal)
	// the transaction can well end before the thread is waiting
	// in the condition variable. As apposed to sempahores
	// pthread_cond don't keep the "credit"
	for(i=0; i < PROMISE_TIMEOUT_S *
			(_PROMISE_1_SEC_NSEC/ PROMISE_RETRY_NSEC) ;++i){
		try{
			if(ret != IPCM_PENDING)
				return ret;
			wait_cond.timedwait(0, PROMISE_RETRY_NSEC);
		}catch(...){};
	}

	//hard timeout expired
	if(!trans->abort())
		//The transaction ended at the very last second
		return ret;
	ret = IPCM_FAILURE;
	return ret;
}

ipcm_res_t Promise::timed_wait(const unsigned int seconds){

	if(ret != IPCM_PENDING)
		return ret;
	try{
		wait_cond.timedwait(seconds, 0);
	}catch (rina::ConcurrentException& e) {
		if(ret != IPCM_PENDING)
			return ret;
		return IPCM_PENDING;
	};
	return ret;
}


//
// Transactions
//

TransactionState::TransactionState(Addon* callee_, Promise* _promise):
					promise(_promise),
					tid(IPCManager->__tid_gen.next()),
					callee(callee_),
					finalised(false){
	if (promise){
		promise->ret = IPCM_PENDING;
		promise->trans = this;
	}
};

//State management routines
int IPCManager_::add_transaction_state(TransactionState* t){

	//Lock
	rina::WriteScopedLock writelock(trans_rwlock);

	//Check if exists already
	if ( pend_transactions.find(t->tid) != pend_transactions.end() ){
		assert(0); //Transaction id repeated
		return -1;
	}

	//Just add and return
	try{
		pend_transactions[t->tid] = t;
	}catch(...){
		LOG_DBG("Could not add transaction %u. Out of memory?", t->tid);
		assert(0);
		return -1;
	}

	return 0;
}

int IPCManager_::remove_transaction_state(int tid){

	TransactionState* t;
	//Lock
	rina::WriteScopedLock writelock(trans_rwlock);

	//Check if it really exists
	if ( pend_transactions.find(tid) == pend_transactions.end() )
		return -1;

	t = pend_transactions[tid];
	pend_transactions.erase(tid);
	delete t;

	return 0;
}

//Syscall state management routines
int IPCManager_::add_syscall_transaction_state(SyscallTransState* t){

	//Lock
	rina::WriteScopedLock writelock(trans_rwlock);

	//Check if exists already
	if ( pend_sys_trans.find(t->tid) != pend_sys_trans.end() ){
		assert(0); //Transaction id repeated
		return -1;
	}

	//Just add and return
	try{
		pend_sys_trans[t->tid] = t;
	}catch(...){
		LOG_DBG("Could not add syscall transaction %u. Out of memory?",
								 t->tid);
		assert(0);
		return -1;
	}

	return 0;
}

int IPCManager_::remove_syscall_transaction_state(int tid){

	SyscallTransState* t;

	//Lock
	rina::WriteScopedLock writelock(trans_rwlock);

	//Check if it really exists
	if ( pend_sys_trans.find(tid) == pend_sys_trans.end() )
		return -1;

	t = pend_sys_trans[tid];
	pend_sys_trans.erase(tid);
	delete t;

	return 0;
}

//Main I/O loop
void IPCManager_::run(){

	void* status;

	//Wait for the request to stop
	stop_cond.doWait();

	//Cleanup
	LOG_DBG("Cleaning the house...");

	//Destroy all addons (stop them)
	Addon::destroy_all();

	//Destroy all IPCPs
	std::vector<IPCMIPCProcess *> ipcps;
	ipcp_factory_.listIPCProcesses(ipcps);
	std::vector<IPCMIPCProcess *>::const_iterator it;

	//Rwlock: write
	for(it = ipcps.begin(); it != ipcps.end(); ++it){
		if(destroy_ipcp(NULL, (*it)->get_id()) < 0 ){
			LOG_WARN("Warning could not destroy IPCP id: %d\n",
								(*it)->get_id());
		}
	}

	//Join the I/O loop thread
	keep_running = false;
	io_thread->join(&status);

	//I/O thread
	delete io_thread;
}

//static
void* IPCManager_::io_loop_trampoline(void* param){
	IPCManager->io_loop();
	return NULL;
}

void IPCManager_::io_loop(){
	rina::IPCEvent *event;

	keep_running = true;

	LOG_DBG("Starting main I/O loop...");

	while(keep_running) {
		event = rina::ipcEventProducer->eventTimedWait(
						IPCM_EVENT_TIMEOUT_S,
						IPCM_EVENT_TIMEOUT_NS);
		if(req_to_stop){
			//Signal the main thread to start
			//the stop procedure
			stop_cond.signal();
		}

		if(!event)
			continue;

		if (!keep_running)
			break;

		LOG_DBG("Got event of type %s and sequence number %u",
		rina::IPCEvent::eventTypeToString(event->eventType).c_str(),
							event->sequenceNumber);

		try {
			switch(event->eventType){
				case rina::FLOW_ALLOCATION_REQUESTED_EVENT:
						{
						DOWNCAST_DECL(event, rina::FlowRequestEvent, e);
						flow_allocation_requested_event_handler(e);
						}
						break;

				case rina::ALLOCATE_FLOW_RESPONSE_EVENT:
						{
        					DOWNCAST_DECL(event, rina::AllocateFlowResponseEvent, e);
						allocate_flow_response_event_handler(e);
						}
						break;

				case rina::FLOW_DEALLOCATION_REQUESTED_EVENT:
						{
						DOWNCAST_DECL(event, rina::FlowDeallocateRequestEvent, e);
						flow_deallocation_requested_event_handler(e);
						}
						break;

				case rina::FLOW_DEALLOCATED_EVENT:
						{
						DOWNCAST_DECL(event, rina::FlowDeallocatedEvent, e);
						IPCManager->flow_deallocated_event_handler(e);
						}
						break;
				case rina::APPLICATION_REGISTRATION_REQUEST_EVENT:
						{
        					DOWNCAST_DECL(event, rina::ApplicationRegistrationRequestEvent, e);
						app_reg_req_handler(e);
						}
						break;

				case rina::APPLICATION_UNREGISTRATION_REQUEST_EVENT:
						{
        					DOWNCAST_DECL(event, rina::ApplicationUnregistrationRequestEvent, e);
						application_unregistration_request_event_handler(e);
						}
						break;

				case rina::ASSIGN_TO_DIF_RESPONSE_EVENT:
						{
        					DOWNCAST_DECL(event, rina::AssignToDIFResponseEvent, e);
						assign_to_dif_response_event_handler(e);
						}
						break;

				case rina::UPDATE_DIF_CONFIG_RESPONSE_EVENT:
						{
        					DOWNCAST_DECL(event, rina::UpdateDIFConfigurationResponseEvent, e);
						update_dif_config_response_event_handler(e);
						}
						break;

				case rina::ENROLL_TO_DIF_RESPONSE_EVENT:
						{
        					DOWNCAST_DECL(event, rina::EnrollToDIFResponseEvent, e);
						enroll_to_dif_response_event_handler(e);
						}
						break;

				case rina::OS_PROCESS_FINALIZED:
						{
						DOWNCAST_DECL(event, rina::OSProcessFinalizedEvent, e);
						os_process_finalized_handler(e);
						}
						break;

				case rina::IPCM_REGISTER_APP_RESPONSE_EVENT:
						{
        					DOWNCAST_DECL(event, rina::IpcmRegisterApplicationResponseEvent, e);
						app_reg_response_handler(e);
						}
						break;

				case rina::IPCM_UNREGISTER_APP_RESPONSE_EVENT:
						{
        					DOWNCAST_DECL(event, rina::IpcmUnregisterApplicationResponseEvent, e);
						unreg_app_response_handler(e);
						}
						break;

				case rina::IPCM_DEALLOCATE_FLOW_RESPONSE_EVENT:
						{
        					DOWNCAST_DECL(event, rina::IpcmDeallocateFlowResponseEvent, e);
						ipcm_deallocate_flow_response_event_handler(e);
						}
						break;

				case rina::IPCM_ALLOCATE_FLOW_REQUEST_RESULT:
						{
        					DOWNCAST_DECL(event, rina::IpcmAllocateFlowRequestResultEvent, e);
						ipcm_allocate_flow_request_result_handler(e);
						}
						break;

				case rina::QUERY_RIB_RESPONSE_EVENT:
						{
						DOWNCAST_DECL(event, rina::QueryRIBResponseEvent, e);
						query_rib_response_event_handler(e);
						}
						break;

				case rina::IPC_PROCESS_DAEMON_INITIALIZED_EVENT:
						{
						DOWNCAST_DECL(event, rina::IPCProcessDaemonInitializedEvent, e);
						ipc_process_daemon_initialized_event_handler(e);
						}
						break;

				//Policies
				case rina::IPC_PROCESS_SET_POLICY_SET_PARAM_RESPONSE:
					{
        				DOWNCAST_DECL(event, rina::SetPolicySetParamResponseEvent, e);
					ipc_process_set_policy_set_param_response_handler(e);
					}
					break;
				case rina::IPC_PROCESS_SELECT_POLICY_SET_RESPONSE:
					{
        				DOWNCAST_DECL(event, rina::SelectPolicySetResponseEvent, e);
					ipc_process_select_policy_set_response_handler(e);
					}
					break;
				case rina::IPC_PROCESS_PLUGIN_LOAD_RESPONSE:
					{
        				DOWNCAST_DECL(event, rina::PluginLoadResponseEvent, e);
					ipc_process_plugin_load_response_handler(e);
					}
					break;

				//Addon specific events
				default:
					{
					TransactionState* trans = get_transaction_state<TransactionState>(
								  event->sequenceNumber);

					Addon::distribute_flow_event(event);

					if (trans) {
						//Mark as completed
						trans->completed(IPCM_SUCCESS);
						remove_transaction_state(trans->tid);
					}

					continue;
					break;
					}
			}

		} catch (rina::Exception &e) {
			LOG_ERR("ERROR while processing event: %s", e.what());
			//TODO: move locking to a smaller scope
		}

		delete event;
	}

	//TODO: probably move this to a private method if it starts to grow
	LOG_DBG("Stopping I/O loop...");

}

} //rinad namespace
