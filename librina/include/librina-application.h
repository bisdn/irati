/*
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

#ifndef LIBRINA_APPLICATION_H
#define LIBRINA_APPLICATION_H

#ifdef __cplusplus

#include <map>

#include "librina-common.h"
#include "patterns.h"
#include "concurrency.h"

/**
 * The librina-application library provides the native RINA API,
 * allowing applications to i) express their availability to be
 * accessed through one or more DIFS (application registration);
 * ii) allocate and deallocate flows to destination applications
 * (flow allocation and deallocation); iii) read and write data
 * from/to allocated flows (in the form of Service Data Units or
 * SDUs) and iv) query the DIFs available in the system and
 * their properties.
 *
 * For the "slow-path" operations, librina-application interacts
 * with the RINA daemons in by exchanging messages over Netlink
 * sockets. In the case of the "fast-path" operations - i.e. those
 * that need to be invoked for every single SDU: read and write -
 * librina-application communicates directly with the services
 * provided by the kernel through the use of system calls.
 *
 * The librina-application API is event-based; that is: the API
 * provides a different method for each action that can be invoked
 * (allocate_flow, register_application and so on), but only two
 * methods - one blocking, the other non-blocking - to get the
 * results of the operations and SDUs available to be read
 * (event_wait and event_poll).
 */
namespace rina {

enum FlowState {
	FLOW_ALLOCATED, FLOW_DEALLOCATION_REQUESTED, FLOW_DEALLOCATED
};

/**
 * Thrown when some operation is invoked in a flow that is not allocated
 */
class FlowNotAllocatedException: public IPCException {
public:
	FlowNotAllocatedException():
		IPCException(
				"Invalid operation invoked when the flow is not allocated "){
	}
	FlowNotAllocatedException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems reading SDUs
 */
class ReadSDUException: public IPCException {
public:
	ReadSDUException():
		IPCException("Problems reading SDU from flow"){
	}
	ReadSDUException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems writing SDUs
 */
class WriteSDUException: public IPCException {
public:
	WriteSDUException():
		IPCException("Problems writing SDU to flow"){
	}
	WriteSDUException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems registering an application to a DIF
 */
class ApplicationRegistrationException: public IPCException {
public:
	ApplicationRegistrationException():
		IPCException("Problems registering application to DIF"){
	}
	ApplicationRegistrationException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems unregistering an application from a DIF
 */
class ApplicationUnregistrationException: public IPCException {
public:
	ApplicationUnregistrationException():
		IPCException("Problems unregistering application from DIF"){
	}
	ApplicationUnregistrationException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems allocating a flow
 */
class FlowAllocationException: public IPCException {
public:
	FlowAllocationException():
		IPCException("Problems allocating flow"){
	}
	FlowAllocationException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems deallocating a flow
 */
class FlowDeallocationException: public IPCException {
public:
	FlowDeallocationException():
		IPCException("Problems deallocating flow"){
	}
	FlowDeallocationException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Thrown when there are problems querying DIF properties
 */
class GetDIFPropertiesException: public IPCException {
public:
	GetDIFPropertiesException():
		IPCException("Problems getting DIF properties"){
	}
	GetDIFPropertiesException(const std::string& description):
		IPCException(description){
	}
};

/**
 * Represents a flow between two application processes, and encapsulates
 * the services that the flow provides.
 */
class Flow {
	/** The port-id that locally identifies the flow */
	int portId;

	/** The name of the DIF that is providing this flow */
	ApplicationProcessNamingInformation DIFName;

	/** The local application name */
	ApplicationProcessNamingInformation localApplicationName;

	/** The remote by the flow */
	ApplicationProcessNamingInformation remoteApplicationName;

	/** The characteristics of the flow */
	FlowSpecification flowSpecification;

	/** The state of the flow */
	FlowState flowState;

	Flow(const ApplicationProcessNamingInformation& localApplicationName,
	     const ApplicationProcessNamingInformation& remoteApplicationName,
	     const FlowSpecification& flowSpecification, FlowState flowState);

	Flow(const ApplicationProcessNamingInformation& localApplicationName,
	     const ApplicationProcessNamingInformation& remoteApplicationName,
	     const FlowSpecification& flowSpecification, FlowState flowState,
	     const ApplicationProcessNamingInformation& DIFName, int portId);

	void setPortId(int portId);
	void setDIFName(const ApplicationProcessNamingInformation& DIFName);
	void setState(FlowState flowState);

public:
	Flow();
	const FlowState& getState() const;
	int getPortId() const;
	const ApplicationProcessNamingInformation& getDIFName() const;
	const ApplicationProcessNamingInformation& getLocalApplicationName() const;
	const ApplicationProcessNamingInformation& getRemoteApplcationName() const;
	const FlowSpecification getFlowSpecification() const;
	bool isAllocated() const;

	/**
	 * Reads an SDU from the flow. This function will block until there is an
	 * SDU available.
	 *
	 * @param sdu A buffer to store the SDU data
	 * @param maxBytes The maximum number of bytes to read
	 * @return int The number of bytes read
	 * @throws IPCException if the flow is not in the ALLOCATED state
	 */
	int readSDU(void * sdu, int maxBytes)
			throw (FlowNotAllocatedException, ReadSDUException);

	/**
	 * Writes an SDU to the flow
	 *
	 * @param sdu A buffer that contains the SDU data
	 * @param size The size of the SDU data, in bytes
	 * @throws IPCException if the flow is not in the ALLOCATED state or
	 * there are problems writing to the flow
	 */
	void writeSDU(void * sdu, int size)
			throw (FlowNotAllocatedException, WriteSDUException);

	friend class IPCManager;
};

/**
 * Contains the information about a registered application: its
 * name and the DIFs where it is registered
 */
class ApplicationRegistration {
	/** The registered application name */
	ApplicationProcessNamingInformation applicationName;

	/** The list of one or more DIFs in which the application is registered */
	std::list<ApplicationProcessNamingInformation> DIFNames;

public:
	ApplicationRegistration(
			const ApplicationProcessNamingInformation& applicationName);
	const ApplicationProcessNamingInformation& getApplicationName() const;
	const std::list<ApplicationProcessNamingInformation>& getDIFNames() const;
	void addDIFName(const ApplicationProcessNamingInformation& DIFName);
	void removeDIFName(const ApplicationProcessNamingInformation& DIFName);
};

/**
 * Point of entry to the IPC functionality available in the system. This class
 * is a singleton.
 */
class IPCManager : public Lockable{
	/** The flows that are currently allocated */
	std::map<int, Flow*> allocatedFlows;

	/** The flows that are currently allocated */
	std::map<unsigned int, Flow*> pendingFlows;

	/** The flows that are currently allocated */
	std::map<unsigned int, ApplicationRegistrationInformation*>
	        registrationInformation;

	/** The applications that are currently registered in one or more DIFs */
	std::map<ApplicationProcessNamingInformation,
	        ApplicationRegistration*> applicationRegistrations;

	/** Return the pending flow at sequenceNumber */
	Flow * getPendingFlow(unsigned int seqNumber);

	/** Return the allocated flow at portId */
	Flow * getAllocatedFlow(int portId);

	/** Return the information of a registration request */
	ApplicationRegistrationInformation * getRegistrationInfo(
	                unsigned int seqNumber) throw (IPCException);

	ApplicationRegistration * getApplicationRegistration(
	                const ApplicationProcessNamingInformation& appName);
public:
	IPCManager();
	~IPCManager() throw();
	static const std::string application_registered_error;
	static const std::string application_not_registered_error;
	static const std::string unknown_flow_error;
	static const std::string error_registering_application;
	static const std::string error_unregistering_application;
	static const std::string error_requesting_flow_allocation;
	static const std::string error_requesting_flow_deallocation;
	static const std::string error_getting_dif_properties;
	static const std::string wrong_flow_state;

	/**
	 * Retrieves the names and characteristics of a single DIF or of all the
	 * DIFs available to the application.
	 *
	 * @param applicationName The name of the application that wants to query
	 * the properties of one or more DIFs
	 * @param DIFName If provided, the function will return the information of
	 * the requested DIF, otherwise it will return the properties of all the
	 * DIFs available to the application.
	 * @return A handler to be able to identify the proper response event
	 * @throws GetDIFPropertiesException
	 */
	unsigned int getDIFProperties(
			const ApplicationProcessNamingInformation& applicationName,
			const ApplicationProcessNamingInformation& DIFName)
			throw (GetDIFPropertiesException);

	/**
	 * Requests an application to be registered in a DIF
	 *
	 * @param appRegistrationInfo Information about the registration request
	 * (what application, how many DIFs, what specific DIFs)
	 * @throws ApplicationRegistrationException
	 * @return A handler to be able to identify the proper response event
	 */
	unsigned int requestApplicationRegistration(
			ApplicationRegistrationInformation * appRegistrationInfo)
			throw (ApplicationRegistrationException);

	/**
	 * The application registration has been successful,
	 * update data structures
	 * @param seqNumber the id of the registration request
	 * @param DIFName the DIF where the application has been registered
	 * @throws ApplicationRegistrationException if there are issues
	 * registering the application
	 * @return the information on the application registration
	 */
	ApplicationRegistration * commitPendingResitration(
	                unsigned int seqNumber,
	                const ApplicationProcessNamingInformation& DIFName)
	throw (ApplicationRegistrationException);

	/**
	 * The application registration has been unsuccessful,
	 * update data structures
	 * @param seqNumber the if of the registration request
	 * @throws ApplicationRegistrationException if the pending registration
	 * is not found
	 */
	void withdrawPendingRegistration(unsigned int seqNumber)
	throw (ApplicationRegistrationException);

	/**
	 * Requests an application to be unregistered from a DIF
	 *
	 * @param applicationName The name of the application to be unregistered
	 * @param DIFName Then name of the DIF where the application has to be
	 * unregistered from
	 * @return A handler to be able to identify the proper response event
	 * @throws ApplicationUnregistrationException
	 */
	unsigned int requestApplicationUnregistration(
			ApplicationProcessNamingInformation applicationName,
			ApplicationProcessNamingInformation DIFName)
			throw (ApplicationUnregistrationException);

	/**
	 * Inform about the result of a pending application unregistration request
	 * @param seqNumber the id of the request
	 * @param success true if request was successful, false otherwise
	 */
	void appUnregistrationResult(unsigned int seqNumber, bool success)
	                        throw (ApplicationUnregistrationException);

	/**
	 * Requests the allocation of a Flow
	 *
	 * @param localAppName The naming information of the local application
	 * @param remoteAppName The naming information of the remote application
	 * @param flowSpecifiction The characteristics required for the flow
	 * @return A handler to be able to identify the proper response event
	 * @throws FlowAllocationException if there are problems during the flow allocation
	 */
	unsigned int requestFlowAllocation(
			const ApplicationProcessNamingInformation& localAppName,
			const ApplicationProcessNamingInformation& remoteAppName,
			const FlowSpecification& flow) throw (FlowAllocationException);

	/**
	 * Tell the IPC Manager that a pending flow has been allocated, and
	 * get the flow structure
	 * @param sequenceNumber the handler of the pending flow
	 * @param portId the portId that has been allocated to the pending flow
	 * @param DIFName the name of the DIF where the flow has been allocated
	 * @return the flow, ready to be used
	 * @throws FlowAllocationException if the pending flow is not found
	 */
	Flow * commitPendingFlow(unsigned int sequenceNumber, int portId,
	                const ApplicationProcessNamingInformation& DIFName)
	        throw (FlowAllocationException);

	/**
	 * Tell the IPC Manager that a pending flow allocation has been denied
	 * @param sequenceNumber the handler of the pending flow
	 * @throws FlowAllocationException if the pending flow is not found
	 */
	void withdrawPendingFlow(unsigned int sequenceNumber)
	        throw (FlowAllocationException);

	/**
	 * Confirms or denies the request for a flow to this application.
	 *
	 * @param flowRequestEvent information of the flow request
	 * @param result 0 means the flow is accepted, a different number
	 * indicates the deny code
	 * @param notifySource if true the source IPC Process will get
	 * the allocate flow response message back, otherwise it will be ignored
	 * @return Flow If the flow is accepted, returns the flow object
	 * @throws FlowAllocationException If there are problems
	 * confirming/denying the flow
	 */
	Flow * allocateFlowResponse(const FlowRequestEvent& flowRequestEvent,
			int result, bool notifySource)
			throw (FlowAllocationException);

	/**
	 * Requests the deallocation of a flow
	 *
	 * @param portId, the portId of the flow to be deallocated
	 * @throws FlowDeallocationException if the flow is not in
	 * the ALLOCATED state or there are problems deallocating the flow
	 */
	unsigned int requestFlowDeallocation(int portId)
			throw (FlowDeallocationException);

	/**
	 * Inform about the success/failure of a flow deallocation request
	 * @param success true if request has been successful, false otherwise
	 * @param portId the portId of the flow to be deallocated
	 * @throws flowDeallocationException if there are problems
	 */
	void flowDeallocationResult(int portId, bool success)
	                throw (FlowDeallocationException);

	/**
	 * Returns the flows that are currently allocated
	 *
	 * @return the flows allocated
	 */
	std::vector<Flow *> getAllocatedFlows();

	/**
	 * Returns the applications that are currently registered in one or more
	 * DIFs.
	 *
	 * @return the registered applications
	 */
	std::vector<ApplicationRegistration *> getRegisteredApplications();
};

/**
 * Make IPCManager singleton
 */
extern Singleton<IPCManager> ipcManager;

/**
 * Event informing that an application has been unregistered from a DIF,
 * without the application having requested it
 */
class ApplicationUnregisteredEvent: public IPCEvent {
	/** The application that has been unregistered */
	ApplicationProcessNamingInformation applicationName;

	/** The DIF from which the application has been unregistered */
	ApplicationProcessNamingInformation DIFName;

public:
	ApplicationUnregisteredEvent(
			const ApplicationProcessNamingInformation& appName,
			const ApplicationProcessNamingInformation& DIFName,
			unsigned int sequenceNumber);
	const ApplicationProcessNamingInformation& getApplicationName() const;
	const ApplicationProcessNamingInformation& getDIFName() const;
};

/**
 * Event informing that an application registration has been canceled
 * without the application having requested it
 */
class AppRegistrationCanceledEvent: public IPCEvent {

	/** The application whose registration has been canceled */
	ApplicationProcessNamingInformation applicationName;

	/** The name of the DIF */
	ApplicationProcessNamingInformation difName;

	/** An error code indicating why the flow was deallocated */
	int code;

	/** Optional explanation giving more details about why the application registration has been canceled */
	std::string reason;
public:
	AppRegistrationCanceledEvent(int code, const std::string& reason,
			const ApplicationProcessNamingInformation& difName,
			unsigned int sequenceNumber);
	int getCode() const;
	const std::string getReason() const;
	const ApplicationProcessNamingInformation& getApplicationName() const;
	const ApplicationProcessNamingInformation getDIFName() const;
};

/**
 * Event informing about the result of a flow allocation request
 */
class AllocateFlowRequestResultEvent: public IPCEvent {
        /** The application that requested the flow allocation */
        ApplicationProcessNamingInformation sourceAppName;

        /**
         * The port-id assigned to the flow, or error code if the value is
         * negative
         */
        int portId;

        /**
         * The DIF where the flow has been allocated
         */
        ApplicationProcessNamingInformation difName;

public:
        AllocateFlowRequestResultEvent(
                        const ApplicationProcessNamingInformation& appName,
                        const ApplicationProcessNamingInformation& difName,
                        int portId, unsigned int sequenceNumber);
        const ApplicationProcessNamingInformation& getAppName() const;
        const ApplicationProcessNamingInformation& getDIFName() const;
        int getPortId() const;
};

/**
 * Event informing about the result of a flow deallocation request
 */
class DeallocateFlowResponseEvent: public BaseResponseEvent {
        /** The application that requested the flow deallocation */
        ApplicationProcessNamingInformation appName;

public:
        DeallocateFlowResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        int result, unsigned int sequenceNumber);
        const ApplicationProcessNamingInformation& getAppName() const;
};

/**
 * Event informing about the result of a get DIF properties operation
 */
class GetDIFPropertiesResponseEvent: public BaseResponseEvent {
        /**
         * The name of the application that is querying the DIF properties
         */
        ApplicationProcessNamingInformation applicationName;

        /** The properties of zero or more DIFs */
        std::list<DIFProperties> difProperties;

public:
        GetDIFPropertiesResponseEvent(
                        const ApplicationProcessNamingInformation& appName,
                        const std::list<DIFProperties>& difProperties,
                        int result, unsigned int sequenceNumber);
        const ApplicationProcessNamingInformation& getAppName() const;
        const std::list<DIFProperties>& getDIFProperties() const;
};

}

#endif

#endif
