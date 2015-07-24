/**
 * @file ribdv1.h
 * @author Bernat Gaston <bernat (dot) gaston (at) i2cat (dot) net>
 *
 * @brief Management Agent RIB daemon v1
 */


#ifndef __RINAD_IPCP_OBJ_H__
#define __RINAD_IPCP_OBJ_H__

#include <librina/rib_v2.h>
#include <librina/patterns.h>
#include <librina/common.h>

//Encoders and structs
#include "encoders_mad.h"

namespace rinad{
namespace mad{
namespace rib_v1{

//fwd decl
class IPCPObj;

/**
 * IPCP object
 */
class IPCPObj : public DelegationObj{

public:
	IPCPObj(int ipcp_id);
	virtual ~IPCPObj(){};

	const std::string& get_class() const{
		return class_name;
	};

	///
	/// Remote invocations, resulting from CDAP messages
	///

	// Create
	void create(const cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const cdap_rib::filt_info_t &filt,
				const int invoke_id,
				const cdap_rib::SerializedObject &obj_req,
				cdap_rib::SerializedObject &obj_reply,
				cdap_rib::res_info_t& res);

	// Delete
	bool delete_(const cdap_rib::con_handle_t &con,
					const std::string& fqn,
					const std::string& class_,
					const cdap_rib::filt_info_t &filt,
					const int invoke_id,
					cdap_rib::res_info_t& res);

	// Read
	void read(const cdap_rib::con_handle_t &con,
					const std::string& fqn,
					const std::string& class_,
					const cdap_rib::filt_info_t &filt,
					const int invoke_id,
					cdap_rib::SerializedObject &obj_reply,
					cdap_rib::res_info_t& res);

	// Write
	void write(const cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const cdap_rib::filt_info_t &filt,
				const int invoke_id,
				const cdap_rib::SerializedObject &obj_req,
				cdap_rib::SerializedObject &obj_reply,
				cdap_rib::res_info_t& res);

	// Start
	void start(const cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const cdap_rib::filt_info_t &filt,
				const int invoke_id,
				const cdap_rib::SerializedObject &obj_req,
				cdap_rib::SerializedObject &obj_reply,
				cdap_rib::res_info_t& res);

	// Stop
	void stop(const cdap_rib::con_handle_t &con,
				const std::string& fqn,
				const std::string& class_,
				const cdap_rib::filt_info_t &filt,
				const int invoke_id,
				const cdap_rib::SerializedObject &obj_req,
				cdap_rib::SerializedObject &obj_reply,
				cdap_rib::res_info_t& res);

	//Name of the class
	const static std::string class_name;

	//Process ID
	int processID_;

protected:
	static int createIPCP(rinad::mad_manager::structures::ipcp_config_t &object);
	static bool assignToDIF(rinad::mad_manager::structures::ipcp_config_t &object, int ipcp_id);
	static bool registerAtDIFs(rinad::mad_manager::structures::ipcp_config_t &object, int ipcp_id);

	//Mapping internal invoke_id <-> external invoke_id
	static std::map<int, int> invoke_id_mappings;

private:
	mad_manager::encoders::IPCPEncoder encoder;
};

}; //namespace rib_v1
}; //namespace mad
}; //namespace rinad


#endif  /* __RINAD_IPCP_OBJ_H__ */
