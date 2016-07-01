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
	StreamsDoneCallBackInterface(),
	m_shortproc(dataSddsOut, this),
	m_floatproc(dataSddsOut, this),
	m_octetproc(dataSddsOut, this)
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
	setPropertyQueryImpl(status, this, &SinkSDDS_i::get_status_struct);

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
 * Used as a callback for any of the stream processors. Used only to kick off any stream in the on deck pile
 */
void SinkSDDS_i::streamsDone(std::string streamId) {
	boost::unique_lock<boost::mutex> lock(m_stream_lock);
	LOG_INFO(SinkSDDS_i, "Completed processing: " << streamId);
	if (m_floatStreamOnDeck.size()) {
		bulkio::InFloatStream newFloatStream = m_floatStreamOnDeck.back();
		newFloatStream.enable();
		m_floatproc.setStream(newFloatStream);
		m_floatStreamOnDeck.clear();
		if (started()) { m_floatproc.run(); }
	} else if (m_shortStreamOnDeck.size()) {
		bulkio::InShortStream newShortStream = m_shortStreamOnDeck.back();
		newShortStream.enable();
		m_shortproc.setStream(newShortStream);
		m_shortStreamOnDeck.clear();
		if (started()) { m_shortproc.run(); }
	} else if (m_octetStreamOnDeck.size()) {
		bulkio::InOctetStream newOctetStream = m_octetStreamOnDeck.back();
		newOctetStream.enable();
		m_octetproc.setStream(newOctetStream);
		m_octetStreamOnDeck.clear();
		if (started()) { m_octetproc.run(); }
	}
}

/**
 * Call back for a new stream coming into the float port. When a new stream
 * arrives, it is only accepted if no other stream processor is active as this
 * component only handles a single stream -> SDDS stream at a time.
 */
void SinkSDDS_i::setFloatStream(bulkio::InFloatStream floatStream) {
	boost::unique_lock<boost::mutex> lock(m_stream_lock);

	if (streamIsActive()) {
		LOG_WARN(SinkSDDS_i, "Cannot create new stream there is already an active stream. Disabling stream and adding it to an on deck list: " << floatStream.streamID());
		floatStream.disable();
		if (streamIsOnDeck()) {
			LOG_WARN(SinkSDDS_i, "There is already a stream on deck, the new stream will take precedence.");
			dropOnDeckStream();
		}

		m_floatStreamOnDeck.push_back(floatStream);
	} else {
		LOG_DEBUG(SinkSDDS_i, "Passing new float stream to proc: " << floatStream.streamID());
		m_floatproc.setStream(floatStream);
		if (started()) { m_floatproc.run();}
	}
}

/**
 * Call back for a new stream coming into the short port. When a new stream
 * arrives, it is only accepted if no other stream processor is active as this
 * component only handles a single stream -> SDDS stream at a time.
 */
void SinkSDDS_i::setShortStream(bulkio::InShortStream shortStream) {
	boost::unique_lock<boost::mutex> lock(m_stream_lock);

	if (streamIsActive()) {
		LOG_WARN(SinkSDDS_i, "Cannot create new stream there is already an active stream. Disabling stream and adding it to an on deck list: " << shortStream.streamID());
		shortStream.disable();
		if (streamIsOnDeck()) {
			LOG_WARN(SinkSDDS_i, "There is already a stream on deck, the new stream will take precedence.");
			dropOnDeckStream();
		}

		m_shortStreamOnDeck.push_back(shortStream);
	} else {
		LOG_DEBUG(SinkSDDS_i, "Passing new short stream to proc: " << shortStream.streamID());
		m_shortproc.setStream(shortStream);
		if (started()) { m_shortproc.run();}
	}
}

/**
 * Call back for a new stream coming into the octet port. When a new stream
 * arrives, it is only accepted if no other stream processor is active as this
 * component only handles a single stream -> SDDS stream at a time.
 */
void SinkSDDS_i::setOctetStream(bulkio::InOctetStream octetStream) {
	boost::unique_lock<boost::mutex> lock(m_stream_lock);

	if (streamIsActive()) {
		LOG_WARN(SinkSDDS_i, "Cannot create new stream there is already an active stream. Disabling stream and adding it to an on deck list: " << octetStream.streamID());
		octetStream.disable();
		if (streamIsOnDeck()) {
			LOG_WARN(SinkSDDS_i, "There is already a stream on deck, the new stream will take precedence.");
			dropOnDeckStream();
		}

		m_octetStreamOnDeck.push_back(octetStream);
	} else {
		LOG_DEBUG(SinkSDDS_i, "Passing new octet stream to proc: " << octetStream.streamID());
		m_octetproc.setStream(octetStream);
		if (started()) { m_octetproc.run();}
	}
}

/**
 * True if there is a stream currently "on deck"
 */
bool SinkSDDS_i::streamIsOnDeck() {
	return m_floatStreamOnDeck.size() > 0 || m_shortStreamOnDeck.size() > 0 || m_octetStreamOnDeck.size() > 0;
}


/**
 * True if there is a stream is currently active
 */
bool SinkSDDS_i::streamIsActive() {
	return m_floatproc.isActive() || m_shortproc.isActive() || m_octetproc.isActive();
}


/**
 * Drops the stream on deck and logs it.
 */
void SinkSDDS_i::dropOnDeckStream() {
	if (m_floatStreamOnDeck.size()) {
		LOG_WARN(SinkSDDS_i, "Dropping float stream: " << m_floatStreamOnDeck[0].streamID());
		m_floatStreamOnDeck.clear();
	}
	if (m_shortStreamOnDeck.size()) {
		LOG_WARN(SinkSDDS_i, "Dropping short stream: " << m_shortStreamOnDeck[0].streamID());
		m_shortStreamOnDeck.clear();
	}
	if (m_octetStreamOnDeck.size()) {
		LOG_WARN(SinkSDDS_i, "Dropping octet stream: " << m_octetStreamOnDeck[0].streamID());
		m_octetStreamOnDeck.clear();
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
 * Gets the status struct which contains the current and on deck stream IDs.
 */
struct status_struct SinkSDDS_i::get_status_struct() {
	boost::unique_lock<boost::mutex> lock(m_stream_lock);

	struct status_struct retVal;
	retVal.current_stream = "";
	retVal.stream_on_deck = "";

	if (m_floatproc.isActive()) {
		retVal.current_stream = m_floatproc.getStreamId();
	} else if (m_shortproc.isActive()) {
		retVal.current_stream = m_shortproc.getStreamId();
	} else if (m_octetproc.isActive()) {
		retVal.current_stream = m_octetproc.getStreamId();
	}

	if (m_floatStreamOnDeck.size()) {
		retVal.stream_on_deck = m_floatStreamOnDeck.back().streamID();
	} else if (m_shortStreamOnDeck.size()) {
		retVal.stream_on_deck = m_shortStreamOnDeck.back().streamID();
	} else if (m_octetStreamOnDeck.size()) {
		retVal.stream_on_deck = m_octetStreamOnDeck.back().streamID();
	}

	return retVal;
}
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
