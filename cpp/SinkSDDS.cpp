/**************************************************************************

    This is the component code. This file contains the child class where
    custom functionality can be added to the component. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "SinkSDDS.h"
#include <arpa/inet.h>

PREPARE_LOGGING(SinkSDDS_i)

/**
 * Constructor which sets up the stream and connection listeners as well as the property set callbacks.
 * Also zeros out the connection struct.
 *
 */
SinkSDDS_i::SinkSDDS_i(const char *uuid, const char *label) :
    SinkSDDS_base(uuid, label),
	m_shortproc(this, dataSddsOut),
	m_floatproc(this, dataSddsOut),
	m_octetproc(this, dataSddsOut)
{
	dataFloatIn->addStreamListener(this, &SinkSDDS_i::setFloatStream);
	dataFloatIn->removeStreamListener(&m_floatproc, &BulkIOToSDDSProcessor<bulkio::InFloatStream>::removeStream);

	dataShortIn->addStreamListener(this, &SinkSDDS_i::setShortStream);
	dataShortIn->removeStreamListener(&m_shortproc, &BulkIOToSDDSProcessor<bulkio::InShortStream>::removeStream);

	dataOctetIn->addStreamListener(this, &SinkSDDS_i::setOctetStream);
	dataOctetIn->removeStreamListener(&m_octetproc, &BulkIOToSDDSProcessor<bulkio::InOctetStream>::removeStream);

	setPropertyConfigureImpl(sdds_settings, this, &SinkSDDS_i::set_sdds_settings_struct);
	setPropertyConfigureImpl(network_settings, this, &SinkSDDS_i::set_network_settings_struct);
	setPropertyConfigureImpl(override_sdds_header, this, &SinkSDDS_i::set_override_sdds_header_struct);

	dataSddsOut->setNewConnectListener(this, &SinkSDDS_i::newConnectionMade);

	memset(&m_connection, 0, sizeof(m_connection));
}

/**
 * All cleanup occurs in the stop method which is called in both releaseObject and Stop.
 */
SinkSDDS_i::~SinkSDDS_i(){}

/**
 * This is the callback method when a new connection from the SDDS output port is made.
 * Needed for any dynamic connections made while the component is running it will
 * determine the port on the other end of the connection and update the SRI.
 * This is normally done by BulkIO for the other BulkIO style ports but currently (2.0.1) this
 * is not done and has been logged as a bug against the framework. CF-1517
 * It seems as though the attach call is taken care of however.
 */
void SinkSDDS_i::newConnectionMade(const char *connectionId) {
	if (started()) {
		ExtendedCF::UsesConnectionSequence_var currentConnections = dataSddsOut->connections();

		for (size_t i = 0; i < currentConnections->length(); ++i) {
			ExtendedCF::UsesConnection conn = currentConnections[i];
			if (strcmp(connectionId, conn.connectionId) == 0) {
				BULKIO::dataSDDS_var sdds_input_port = BULKIO::dataSDDS::_narrow(conn.port);

				m_shortproc.pushSri(sdds_input_port);
				m_floatproc.pushSri(sdds_input_port);
				m_octetproc.pushSri(sdds_input_port);

			}
		}
	}
}

/**
 * Call back for a new stream coming into the float port. When a new stream
 * arrives, it is only accepted if no other stream processor is active as this
 * component only handles a single stream -> SDDS stream at a time.
 */
void SinkSDDS_i::setFloatStream(bulkio::InFloatStream floatStream) {

	if (getNumberOfActiveStreams() != 1) {
		LOG_WARN(SinkSDDS_i, "Cannot create new stream there is already an active stream. Disabling stream: " << floatStream.streamID());
		floatStream.disable();
	} else {
		LOG_DEBUG(SinkSDDS_i, "Passing new float stream to proc: " << floatStream.streamID());
		m_floatproc.setStream(floatStream);
	}
}

/**
 * Call back for a new stream coming into the short port. When a new stream
 * arrives, it is only accepted if no other stream processor is active as this
 * component only handles a single stream -> SDDS stream at a time.
 */
void SinkSDDS_i::setShortStream(bulkio::InShortStream shortStream) {

	if (getNumberOfActiveStreams() != 1) {
		LOG_WARN(SinkSDDS_i, "Cannot create new stream there is already an active stream. Disabling stream: " << shortStream.streamID());
		shortStream.disable();
	} else {
		LOG_DEBUG(SinkSDDS_i, "Passing new short stream to proc: " << shortStream.streamID());
		m_shortproc.setStream(shortStream);
	}
}

/**
 * Call back for a new stream coming into the octet port. When a new stream
 * arrives, it is only accepted if no other stream processor is active as this
 * component only handles a single stream -> SDDS stream at a time.
 */
void SinkSDDS_i::setOctetStream(bulkio::InOctetStream octetStream) {

	if (getNumberOfActiveStreams() != 1) {
		LOG_WARN(SinkSDDS_i, "Cannot create new stream there is already an active stream. Disabling stream: " << octetStream.streamID());
		octetStream.disable();
	} else {
		LOG_DEBUG(SinkSDDS_i, "Passing new octet stream to proc: " << octetStream.streamID());
		m_octetproc.setStream(octetStream);
	}
}

/**
 * Setter method called by the framework in place of the classic configure call. Will set the
 * override sdds struct property only if the component has not already started. If you need
 * to change these values while running a lock will need to be added.
 */
void SinkSDDS_i::set_override_sdds_header_struct(struct override_sdds_header_struct request) {
	if (started()) {
		LOG_WARN(SinkSDDS_i, "Cannot set the sdds header struct while component is running");
		return;
	}

	override_sdds_header = request;
}

/**
 * Setter method called by the framework in place of the classic configure call. Will set the
 * sdds settings struct only if the component has not already started. If you need
 * to change these values while running a lock will need to be added.
 */
void SinkSDDS_i::set_sdds_settings_struct(struct sdds_settings_struct request) {
	if (started()) {
		LOG_WARN(SinkSDDS_i, "Cannot set the sdds settings while component is running");
		return;
	}

	sdds_settings = request;
}

/**
 * Setter method called by the framework in place of the classic configure call. Will set the
 * network settings struct only if the component has not already started. If you need
 * to change these values while running a lock will need to be added.
 */
void SinkSDDS_i::set_network_settings_struct(struct network_settings_struct request) {
	if (started()) {
		LOG_WARN(SinkSDDS_i, "Cannot set the network settings while component is running");
		return;
	}

	network_settings = request;
}

/**
 * No redhawk specfic constructor code is necessary. All setup is done in the object constructor.
 */
void SinkSDDS_i::constructor(){}

/**
 * During start, the socket is opened and the three bulkIO processors are initialized.
 * If there is an issue establishing the socket, an exception is raised and the bulkIO
 * processors are not started. The starting of the bulkIO processors does nothing if
 * the individual processor has not had a stream set upon it. If everything goes well,
 * the base class' start method is called.
 */
void SinkSDDS_i::start() throw (CORBA::SystemException, CF::Resource::StartError) {
	std::stringstream errorText;
	int socket;

	if (started()) {
		LOG_WARN(SinkSDDS_i, "Already started, call to start ignored.");
		return;
	}

	m_floatproc.setSddsSettings(sdds_settings);
	m_shortproc.setSddsSettings(sdds_settings);
	m_octetproc.setSddsSettings(sdds_settings);

	m_floatproc.setOverrideHeaderSettings(override_sdds_header);
	m_shortproc.setOverrideHeaderSettings(override_sdds_header);
	m_octetproc.setOverrideHeaderSettings(override_sdds_header);

	m_floatproc.setAttachSettings(sdds_attach_settings);
	m_shortproc.setAttachSettings(sdds_attach_settings);
	m_octetproc.setAttachSettings(sdds_attach_settings);

	try {
		socket = setupSocket();
	} catch (...) {
		socket = -1;
	}

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

/**
 * Will tell all three bulkIO to SDDS processors to shutdown, which simply sets the shutDown boolean to true.
 * Will then call the base class stop to open up the ports and free any blocking port read calls.
 * Then the bulkio to sdds processors threads are joined and the socket closed.
 */
void SinkSDDS_i::stop () throw (CF::Resource::StopError, CORBA::SystemException) {
	LOG_TRACE(SinkSDDS_i, "Entering stop method");
	m_floatproc.shutdown();
	m_shortproc.shutdown();
	m_octetproc.shutdown();

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

// TODO: On release maybe call detach for any connections to external applications, unsure if this is taken care of for me.

/**
 * This component does not use the service function and instead sets up its own threads in the start call.
 * Finish is returned on start so that the service function is never called again.
 */
int SinkSDDS_i::serviceFunction()
{
    LOG_DEBUG(SinkSDDS_i, "No service function in SinkSDDS component, returning FINISH");
    return FINISH;
}

/**
 * Attempts to setup either a multicast socket or unicast socket depending on the IP address provided.
 * Returns the socket file descriptor on success and -1 on failure. May also throw an exception in some
 * failure cases so both should be checked for.
 */
int SinkSDDS_i::setupSocket() {
	int retVal = -1;
	std::string interface = network_settings.interface;
	memset(&m_connection, 0, sizeof(m_connection));

	LOG_INFO(SinkSDDS_i, "Setting connection info to Interface: " << network_settings.interface << " IP: " << network_settings.ip_address<< " Port: " << network_settings.port);
	if (network_settings.ip_address.empty()) {
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

/**
 * Counts and returns the number of active streams according to BulkIO. If instead we were to check
 * stream processors, it may result in a race condition if a stream is ended and a new one
 * immediately started. Streams which come in while we are processing an active stream are marked
 * disabled and never serviced.
 */
size_t SinkSDDS_i::getNumberOfActiveStreams() {
	size_t numberOfActiveStreams = 0;
	LOG_INFO(SinkSDDS_i, "Checking how many active streams there are");

	bulkio::InFloatPort::StreamList floatStreamList = dataFloatIn->getStreams();
	bulkio::InFloatPort::StreamList::iterator float_it;

	for (float_it = floatStreamList.begin(); float_it != floatStreamList.end(); ++float_it) {
		if ((*float_it).enabled()) {
			++numberOfActiveStreams;
			LOG_INFO(SinkSDDS_i, "Active float stream found counting it: " << (*float_it).streamID());
		}
	}

	bulkio::InShortPort::StreamList shortStreamList = dataShortIn->getStreams();
	bulkio::InShortPort::StreamList::iterator short_it;
	for (short_it = shortStreamList.begin(); short_it != shortStreamList.end(); ++short_it) {
		if ((*short_it).enabled()) {
			++numberOfActiveStreams;
			LOG_INFO(SinkSDDS_i, "Active short stream found counting it: " << (*short_it).streamID());
		}
	}

	bulkio::InOctetPort::StreamList octetStreamList = dataOctetIn->getStreams();
	bulkio::InOctetPort::StreamList::iterator octet_it;
	for (octet_it = octetStreamList.begin(); octet_it != octetStreamList.end(); ++octet_it) {
		if ((*octet_it).enabled()) {
			++numberOfActiveStreams;
			LOG_INFO(SinkSDDS_i, "Active octet stream found counting it: " << (*octet_it).streamID());
		}
	}

	LOG_INFO(SinkSDDS_i, "Found a total of " << numberOfActiveStreams << " active streams");
	return numberOfActiveStreams;
}
