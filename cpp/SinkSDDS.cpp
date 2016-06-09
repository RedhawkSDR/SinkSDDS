/**************************************************************************

    This is the component code. This file contains the child class where
    custom functionality can be added to the component. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "SinkSDDS.h"
#include <arpa/inet.h>

PREPARE_LOGGING(SinkSDDS_i)

//TODO: Create property that allows the user to optionally disable the SRI pushes out the attach port.
//TODO: deal with the attach call, see if there are any callbacks available and have it call only if started.
//TODO: Try and figure out a better way than calling each method on each processor
SinkSDDS_i::SinkSDDS_i(const char *uuid, const char *label) :
    SinkSDDS_base(uuid, label),
	m_shortproc(this, dataSddsOut),
	m_floatproc(this, dataSddsOut),
	m_octetproc(this, dataSddsOut)
{
	dataFloatIn->addStreamListener(&m_floatproc, &BulkIOToSDDSProcessor<bulkio::InFloatStream>::setStream);
	dataFloatIn->removeStreamListener(&m_floatproc, &BulkIOToSDDSProcessor<bulkio::InFloatStream>::removeStream);

	dataShortIn->addStreamListener(&m_shortproc, &BulkIOToSDDSProcessor<bulkio::InShortStream>::setStream);
	dataShortIn->removeStreamListener(&m_shortproc, &BulkIOToSDDSProcessor<bulkio::InShortStream>::removeStream);

	dataOctetIn->addStreamListener(&m_octetproc, &BulkIOToSDDSProcessor<bulkio::InOctetStream>::setStream);
	dataOctetIn->removeStreamListener(&m_octetproc, &BulkIOToSDDSProcessor<bulkio::InOctetStream>::removeStream);

	setPropertyConfigureImpl(sdds_settings, this, &SinkSDDS_i::set_sdds_settings_struct);
	setPropertyConfigureImpl(network_settings, this, &SinkSDDS_i::set_network_settings_struct);

	// TODO: I should set a connection listener for new connections made during start
//	dataSddsOut->setNewConnectListener(this);

	memset(&m_connection, 0, sizeof(m_connection));
}

SinkSDDS_i::~SinkSDDS_i(){}

void SinkSDDS_i::set_sdds_settings_struct(struct sdds_settings_struct request) {
	if (started()) {
		LOG_WARN(SinkSDDS_i, "Cannot set the sdds settings while component is running");
		return;
	}

	sdds_settings = request;
}

void SinkSDDS_i::set_network_settings_struct(struct network_settings_struct request) {
	if (started()) {
		LOG_WARN(SinkSDDS_i, "Cannot set the network settings while component is running");
		return;
	}

	network_settings = request;
}

void SinkSDDS_i::constructor(){}

void SinkSDDS_i::start() throw (CORBA::SystemException, CF::Resource::StartError) {
	std::stringstream errorText;

	if (started()) {
		LOG_WARN(SinkSDDS_i, "Already started, call to start ignored.");
		return;
	}

	m_floatproc.setSddsSettings(sdds_settings);
	m_shortproc.setSddsSettings(sdds_settings);
	m_octetproc.setSddsSettings(sdds_settings);

	int socket = setupSocket();
	if (socket < 0) {
		errorText << "Could not setup the output socket, cannot start without successful socket connection.";
		LOG_ERROR(SinkSDDS_i, errorText.str());
		throw CF::Resource::StartError(CF::CF_EINVAL, errorText.str().c_str());
	}

	m_octetproc.setConnection(m_connection, network_settings.vlan);
	m_octetproc.run();

	m_shortproc.setConnection(m_connection, network_settings.vlan);
	m_shortproc.run();

	m_floatproc.setConnection(m_connection, network_settings.vlan);
	m_floatproc.run();

	// Call the parent start
	SinkSDDS_base::start();
}

void SinkSDDS_i::stop () throw (CF::Resource::StopError, CORBA::SystemException) {
	LOG_TRACE(SinkSDDS_i, "Entering stop method");
	m_floatproc.shutdown();
	m_shortproc.shutdown();
	m_octetproc.shutdown();

	m_floatproc.callDetach();
	m_shortproc.callDetach();
	m_octetproc.callDetach();

	SinkSDDS_base::stop(); // Opens the port up so that the stream object will return and free up the read lock.

	m_floatproc.join();
	m_shortproc.join();
	m_octetproc.join();

	if (m_connection.sock) {
		close(m_connection.sock);
		memset(&m_connection, 0, sizeof(m_connection));
	}

	LOG_TRACE(SinkSDDS_i, "Exiting stop method");
}

int SinkSDDS_i::serviceFunction()
{
    LOG_DEBUG(SinkSDDS_i, "No service function in SinkSDDS component, returning FINISH");
    return FINISH;
}

int SinkSDDS_i::setupSocket() {
	// TODO: This probably throws an exception that needs to be caught and dealt with.
	int retVal = -1;
	std::string interface = network_settings.interface;
	memset(&m_connection, 0, sizeof(m_connection));

	LOG_INFO(SinkSDDS_i, "Setting connection info to Interface: " << network_settings.interface << " IP: " << network_settings.ip_address<< " Port: " << network_settings.port);
	if (network_settings.ip_address.empty() || network_settings.interface.empty()) {
		LOG_ERROR(SinkSDDS_i, "IP or interface was empty when trying to setup the socket connection.")
		return retVal;
	}

	in_addr_t lowMulti = inet_network("224.0.0.0");
	in_addr_t highMulti = inet_network("239.255.255.250");

	if (network_settings.vlan) {
		std::stringstream ss;
		ss << interface << "." << network_settings.vlan;
		interface = ss.str();
	}

	if ((inet_network(network_settings.ip_address.c_str()) > lowMulti) && (inet_addr(network_settings.ip_address.c_str()) < highMulti)) {
		m_connection = multicast_server(interface.c_str(), network_settings.ip_address.c_str(), network_settings.port);
	} else {
		m_connection = unicast_server(interface.c_str(), network_settings.ip_address.c_str(), network_settings.port);
	}

	LOG_INFO(SinkSDDS_i, "Created socket (fd: " << m_connection.sock << ") connection on: " << interface << " IP: " << network_settings.ip_address << " Port: " << network_settings.port);

	return m_connection.sock;
}
