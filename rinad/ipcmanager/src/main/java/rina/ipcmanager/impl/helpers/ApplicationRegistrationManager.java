package rina.ipcmanager.impl.helpers;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import rina.ipcmanager.impl.IPCManager;

import eu.irati.librina.ApplicationManagerSingleton;
import eu.irati.librina.ApplicationProcessNamingInformation;
import eu.irati.librina.ApplicationRegistrationInformation;
import eu.irati.librina.ApplicationRegistrationRequestEvent;
import eu.irati.librina.ApplicationRegistrationType;
import eu.irati.librina.ApplicationUnregistrationRequestEvent;
import eu.irati.librina.DIFConfiguration;
import eu.irati.librina.IPCProcess;
import eu.irati.librina.IPCProcessFactorySingleton;
import eu.irati.librina.IPCProcessPointerVector;

/**
 * Manages application registrations:
 * 	 a) Validates registration/unregistration requests
 * 	 b) Maintains the state of established registrations
 * 	 c) Keeps track of what DIFs are available to what applications
 * @author eduardgrasa
 *
 */
public class ApplicationRegistrationManager {
	
	private static final Log log = LogFactory.getLog(ApplicationRegistrationManager.class);
	private IPCProcessFactorySingleton ipcProcessFactory = null;
	private ApplicationManagerSingleton applicationManager = null;
	private Map<ApplicationProcessNamingInformation, ApplicationRegistrationState> applicationRegistrations;

	public ApplicationRegistrationManager(IPCProcessFactorySingleton ipcProcessFactory, 
			ApplicationManagerSingleton applicationManager){
		this.ipcProcessFactory = ipcProcessFactory;
		this.applicationManager = applicationManager;
		applicationRegistrations = 
				new ConcurrentHashMap<ApplicationProcessNamingInformation, ApplicationRegistrationState>();
	}
	
	/**
	 * Selects an IPC Process where to register the application, invokes the register application 
	 * method of the IPC Process and updates the aplicationRegistration state
	 * @param event
	 * @throws Exception
	 */
	public synchronized void registerApplication(
			ApplicationRegistrationRequestEvent event) throws Exception{
		IPCProcess ipcProcess = null;
		try{
			ApplicationRegistrationState applicationRegistration = 
					applicationRegistrations.get(event.getApplicationName());
			ipcProcess = getIPCProcessToRegisterAt(event, applicationRegistration);
			ipcProcess.registerApplication(event.getApplicationName());
			if (applicationRegistration == null){
				applicationRegistration = new ApplicationRegistrationState(event.getApplicationName());
				applicationRegistrations.put(event.getApplicationName(), applicationRegistration);
			}

			applicationRegistration.getDIFNames().add(ipcProcess.getConfiguration().getDifName().getProcessName());
		}catch(Exception ex){
			log.error("Error registering application. "+ex.getMessage());
			applicationManager.applicationRegistered(event, new ApplicationProcessNamingInformation(), 
					-1);
			return;
		}
		
		applicationManager.applicationRegistered(event, ipcProcess.getConfiguration().getDifName(), 
				0);
	}
	
	/**
	 * Get application registration (if existent) and cancel it.
	 * @param event
	 * @throws Exception
	 */
	public synchronized void unregisterApplication(
			ApplicationUnregistrationRequestEvent event) throws Exception{
		IPCProcess ipcProcess = null;
		try{
			ApplicationRegistrationState applicationRegistration = 
					applicationRegistrations.get(event.getApplicationName());
			if (applicationRegistration == null){
				throw new Exception("Application "+event.getApplicationName().toString() 
						+ " was not registered to any DIF");
			}
			
			String difName = event.getDIFName().getProcessName();
			if (applicationRegistration.getDIFNames().contains(difName)){
				ipcProcess = selectIPCProcessOfDIF(difName);
				ipcProcess.unregisterApplication(event.getApplicationName());
				applicationRegistration.getDIFNames().remove(difName);
				if (applicationRegistration.getDIFNames().size() == 0){
					applicationRegistrations.remove(event.getApplicationName());
				}
			}else{
				throw new Exception("Application "+ event.getApplicationName().toString() 
						+ " was not registered to DIF "+difName);
			}
		}catch(Exception ex){
			log.error("Error unregistering application. "+ex.getMessage());
			applicationManager.applicationUnregistered(event, -1);
			return;
		}
		
		applicationManager.applicationUnregistered(event, 0);
	}
	
	private IPCProcess getIPCProcessToRegisterAt(ApplicationRegistrationRequestEvent event, 
			ApplicationRegistrationState applicationRegistration) throws Exception{
		ApplicationRegistrationInformation info = event.getApplicationRegistrationInformation();
		
		if (info.getRegistrationType() == ApplicationRegistrationType.APPLICATION_REGISTRATION_ANY_DIF){
			if (applicationRegistration != null){
				throw new Exception("Application already registered in a DIF");
			}
			
			return selectAnyIPCProcess();
		}
		
		if (info.getRegistrationType() == ApplicationRegistrationType.APPLICATION_REGISTRATION_SINGLE_DIF){
			String difName = info.getDIFName().getProcessName();
			if (applicationRegistration != null){
				if (applicationRegistration.getDIFNames().contains(difName)){
					throw new Exception("Application already registered in DIF "+difName);
				}
			}
			
			return selectIPCProcessOfDIF(difName);
		}
		
		throw new Exception("Unsupported registration type: "+info.getRegistrationType());
	}
	
	/**
	 * 1 Look for a normal IPC Process, otherwise for a shim Ethernet one, otherwise
	 * for a shim dummy one
	 * @return
	 */
	private IPCProcess selectAnyIPCProcess() throws Exception{
		IPCProcessPointerVector ipcProcesses = ipcProcessFactory.listIPCProcesses();
		if (ipcProcesses.size() == 0){
			throw new Exception("Could not find IPC Process to register at");
		}
		
		IPCProcess ipcProcess = null;
		for(int i=0; i<ipcProcesses.size(); i++){
			ipcProcess = ipcProcesses.get(i);
			if (ipcProcess.getType().equals(IPCManager.NORMAL_IPC_PROCESS_TYPE)){
				return ipcProcess;
			}
		}
		
		for(int i=0; i<ipcProcesses.size(); i++){
			ipcProcess = ipcProcesses.get(i);
			if (ipcProcess.getType().equals(IPCManager.SHIM_ETHERNET_IPC_PROCESS_TYPE)){
				return ipcProcess;
			}
		}
		
		for(int i=0; i<ipcProcesses.size(); i++){
			ipcProcess = ipcProcesses.get(i);
			if (ipcProcess.getType().equals(IPCManager.SHIM_DUMMY_IPC_PROCESS_TYPE)){
				return ipcProcess;
			}
		}
		
		throw new Exception("Could not find IPC Process to register at");
	}
	
	private IPCProcess selectIPCProcessOfDIF(String difName) throws Exception{
		IPCProcessPointerVector ipcProcesses = ipcProcessFactory.listIPCProcesses();
		IPCProcess ipcProcess = null;
		
		for(int i=0; i<ipcProcesses.size(); i++){
			ipcProcess = ipcProcesses.get(i);
			DIFConfiguration difConfiguration = ipcProcess.getConfiguration();
			if (difConfiguration != null && 
					difConfiguration.getDifName().getProcessName() == difName){
				return ipcProcess;
			}
		}
		
		throw new Exception("Could not find IPC Process belonging to DIF "+difName);
	}

}
