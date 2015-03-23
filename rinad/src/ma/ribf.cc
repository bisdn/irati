#include "ribf.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "agent.h"
#define RINA_PREFIX "mad.ribf"
#include <librina/logs.h>

#include "ribs/ribd_v1.h"

namespace rinad{
namespace mad{

/*
* RIBFactory
*/

//Singleton instance
Singleton<RIBFactory_> RIBFactory;


//Initialization and destruction routines
void RIBFactory_::init(std::list<uint64_t> supported_versions){
	for (std::list<uint64_t>::iterator it = supported_versions.begin();
			it != supported_versions.end(); it++)
	{
		createRIB(*it);
	}

	LOG_DBG("Initialized");
}

void RIBFactory_::destroy(void){

}



//Constructors destructors
RIBFactory_::RIBFactory_(){
	//TODO: register to flow events in librina and spawn workers
}

RIBFactory_::~RIBFactory_() throw (){}

/*
* Inner API
*/
void RIBFactory_::createRIB(uint64_t version){
  // FIXME: change for config param
  std::string com_prot = "GPB";
  char separator = ',';
  cdap_rib::cdap_params_t *params = new cdap_rib::cdap_params_t;
  params->is_IPCP_ = false;
  params->timeout_ = 2000;

  cdap_rib::vers_info_t *vers = new cdap_rib::vers_info_t;
  vers->version_ = (long) version;

	//Serialize
	lock();

	//Check if it exists
	if( rib_inst_.find(version) != rib_inst_.end() ){
		unlock();
		throw eDuplicatedRIB("An instance of the RIB with this version already exists");
	}

	//Create object
	switch(version)
	{
	case 1:
		rib_inst_[version] = factory_.create(RIBDaemonv1->getConnHandler(), RIBDaemonv1->getRespHandler(), com_prot , params, vers, separator);
		break;
	default:
		break;
	}

	//Unlock
	unlock();
}

rib::RIBDInterface& RIBFactory_::getRIB(uint64_t version){

  rib::RIBDInterface* rib;

	//Serialize
	lock();

	//Note: it is safe to recover the RIB reference without a RD lock
	//because removal of RIBs is NOT implemented. However this
	//implementation already protects it
	//Check if it exists
	if( rib_inst_.find(version) == rib_inst_.end() ){
		throw eRIBNotFound("RIB instance not found");
	}

	//TODO: reference count to avoid deletion while being used?

	rib = rib_inst_[version];

	//Unlock
	unlock();

	return *rib;
}


}; //namespace mad
}; //namespace rinad


