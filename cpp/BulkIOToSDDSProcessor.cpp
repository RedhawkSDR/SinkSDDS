/*
 * BulkIOToSDDSProcessor.cpp
 *
 *  Created on: May 26, 2016
 *      Author: ylbagou
 */

#include "BulkIOToSDDSProcessor.h"

PREPARE_LOGGING(BulkIOToSDDSProcessor)

BulkIOToSDDSProcessor::BulkIOToSDDSProcessor(Resource_impl *parent):
m_shutdown(false), m_running(false), m_processorThread(NULL), m_activeStream(NOT_SET), m_parent(parent) {

	m_pkt_template.msg_name = NULL;
	m_pkt_template.msg_namelen = 0;
	m_pkt_template.msg_iov = &m_msg_iov[0];
	m_pkt_template.msg_iovlen = 2; // We use two iov. One is the SDDS header and the second is the payload.
	m_pkt_template.msg_control = NULL;
	m_pkt_template.msg_controllen = 0;
	m_pkt_template.msg_flags = NULL;

	m_msg_iov[0].iov_base = &m_sdds_template;
	m_msg_iov[0].iov_len = SDDS_HEADER_SIZE;
	m_msg_iov[1].iov_base = NULL; // This will be set later.
	m_msg_iov[1].iov_len = SDDS_DATA_SIZE;

	initializeSDDSHeader();
}

void BulkIOToSDDSProcessor::initializeSDDSHeader(){
	m_sdds_template.pp = 0;  //PP Parity Packet
	m_sdds_template.ssd[0] = 0;  //SSD Synchronous Serial Data
	m_sdds_template.aad[0] = 0;  //AAD Asynchronous Auxiliary Data
}

void BulkIOToSDDSProcessor::setSddsSettings(sdds_settings_struct settings) {
	m_sdds_template.sf = settings.standard_format;
	m_sdds_template.of = settings.original_format;
}

BulkIOToSDDSProcessor::~BulkIOToSDDSProcessor() {
	// Lets hope someone released the port lock
	if (m_processorThread) {
		join();
	}
}

void BulkIOToSDDSProcessor::run() {
	if (m_processorThread) {
		if (m_running) {
			LOG_ERROR(BulkIOToSDDSProcessor, "The BulkIO To SDDS Processor is already running, cannot start a new stream without first receiving an EOS!");
			//TODO: Exception!
			return;
		} else {
			join();
		}
	}

	if (m_activeStream == NOT_SET) {
		return;
	}

	m_processorThread = new boost::thread(boost::bind(&BulkIOToSDDSProcessor::_run, this));
}

size_t BulkIOToSDDSProcessor::getDataPointer(char **dataPointer) {
	size_t bytes_read = 0;

	switch (m_activeStream) {
	case FLOAT_STREAM:
		m_floatBlock = m_floatStream.read(SDDS_DATA_SIZE / sizeof(float));
		if (!!m_floatBlock) {  //TODO: Document bang bang
			bytes_read = m_floatBlock.size() * sizeof(float);
			*dataPointer = (char *) m_floatBlock.data();
		}
		break;
	case SHORT_STREAM:
		m_shortBlock = m_shortStream.read(SDDS_DATA_SIZE / sizeof(short));
		if (!!m_shortBlock) {
			bytes_read = m_shortBlock.size() * sizeof(short);
			*dataPointer = (char *) m_shortBlock.data();
		}
		break;
	case OCTET_STREAM:
		m_octetBlock = m_octetStream.read(SDDS_DATA_SIZE / sizeof(char));
		if (!!m_octetBlock) {
			bytes_read = m_octetBlock.size() * sizeof(char);
			*dataPointer = (char *) m_octetBlock.data();
		}
		break;
	case NOT_SET:
		LOG_ERROR(BulkIOToSDDSProcessor, "Processor thread running but the active stream is not set");
		break;
	default:
		LOG_ERROR(BulkIOToSDDSProcessor, "Processor thread running but the active stream is not set");
		break;
	};

	return bytes_read;
}

void BulkIOToSDDSProcessor::_run() {
	m_running = true;
	char *sddsDataBlock;
	size_t bytes_read = 0;

	while (not m_shutdown) {
		bytes_read = getDataPointer(&sddsDataBlock); // TODO: We should have an m_sri that gets passed in here and an sriChanged check.
		if (not bytes_read) {
			//TODO: LOG and send an EOS
			m_activeStream = NOT_SET;
			m_shutdown = true;
			continue;
		} else if (bytes_read != SDDS_DATA_SIZE) {
			// TODO: Should we zero pad and send it out anyway?
			LOG_INFO(BulkIOToSDDSProcessor, "Received " << bytes_read << " rather than the expected " << SDDS_DATA_SIZE << " stream will stop.");
			m_activeStream = NOT_SET;
			m_shutdown = true;
			continue;
		}

		if (sendPacket(sddsDataBlock) < 0) {
			LOG_ERROR(BulkIOToSDDSProcessor, "Failed to push packet over socket, stream will be closed.");
			m_activeStream = NOT_SET;
			m_shutdown = true;
			continue;
		}
	}
	m_shutdown = false;
	m_running = false;
}

int BulkIOToSDDSProcessor::sendPacket(char* dataBlock) {
	ssize_t numSent;
	m_msg_iov[1].iov_base = dataBlock;
	numSent = sendmsg(m_connection.sock, &m_pkt_template, 0);
	LOG_TRACE(BulkIOToSDDSProcessor, "Pushed " << numSent << " bytes out of socket.")
	return numSent;
}

/**
 * Non-blocking and sets the shutdown flag on the processing thread. This will NOT
 * join the thread. It is likely that the thread is stuck in the port
 * read call. A user should call shutDown, then stop the parent componet
 * which will free the port and break the lock so that the thread can continue
 * then the user should call joinThread.
 */
void BulkIOToSDDSProcessor::shutdown() {
	m_shutdown = true;
}

void BulkIOToSDDSProcessor::join() {
	if (m_processorThread) {
		m_processorThread->join();
		delete(m_processorThread);
		m_processorThread = NULL;
	} else {
		LOG_WARN(BulkIOToSDDSProcessor, "Join called but the thread object is null.");
	}
}

void BulkIOToSDDSProcessor::setConnection(connection_t connection) {
	m_connection = connection;
	// Not sure why but the msg_name and msg_namelen must be set. Perhaps it's a UDP thing. I thought they were optional
	m_pkt_template.msg_name = &m_connection.addr;
	m_pkt_template.msg_namelen = sizeof(m_connection.addr);
}















//********************************************************************************************************
// TODO: Feels so gross having essentially the same 2 functions 3 times each. Anyway to fix it?
//********************************************************************************************************

// ***************************** Set Streams ******************************
void BulkIOToSDDSProcessor::setFloatStream(bulkio::InFloatStream stream) {
	boost::unique_lock<boost::mutex> lock(m_streamLock);
	LOG_INFO(BulkIOToSDDSProcessor, "Received new stream: " << stream.streamID());
	if (m_activeStream == NOT_SET) {
		m_activeStream = FLOAT_STREAM;
		m_floatStream = stream;
		m_sdds_template.bps = SDDS_FLOAT_BPS;
		if (m_parent->started()) { run(); }
	} else {
		LOG_ERROR(BulkIOToSDDSProcessor, "There is already an active stream named: " << m_floatStream.streamID() << ", cannot set new stream: " << stream.streamID());
	}
}

void BulkIOToSDDSProcessor::setShortStream(bulkio::InShortStream stream) {
	LOG_INFO(BulkIOToSDDSProcessor, "Received new stream: " << stream.streamID());
	boost::unique_lock<boost::mutex> lock(m_streamLock);
	if (m_activeStream == NOT_SET) {
		m_activeStream = SHORT_STREAM;
		m_shortStream = stream;
		m_sdds_template.bps = SDDS_SHORT_BPS;
		if (m_parent->started()) { run(); }
	} else {
		LOG_ERROR(BulkIOToSDDSProcessor, "There is already an active stream named: " << m_shortStream.streamID() << ", cannot set new stream: " << stream.streamID());
	}
}

void BulkIOToSDDSProcessor::setOctetStream(bulkio::InOctetStream stream) {
	LOG_INFO(BulkIOToSDDSProcessor, "Received new stream: " << stream.streamID());
	boost::unique_lock<boost::mutex> lock(m_streamLock);
	if (m_activeStream == NOT_SET) {
		m_activeStream = OCTET_STREAM;
		m_octetStream = stream;
		m_sdds_template.bps = SDDS_OCTET_BPS;
		if (m_parent->started()) { run(); }
	} else {
		LOG_ERROR(BulkIOToSDDSProcessor, "There is already an active stream named: " << m_octetStream.streamID() << ", cannot set new stream: " << stream.streamID());
	}
}




// ***************************** Remove Streams ******************************

void BulkIOToSDDSProcessor::removeFloatStream(bulkio::InFloatStream stream) {
	LOG_INFO(BulkIOToSDDSProcessor, "Removing stream: " << stream.streamID());
	boost::unique_lock<boost::mutex> lock(m_streamLock);
	if (m_activeStream == FLOAT_STREAM && m_floatStream.streamID() == stream.streamID()) {
		m_activeStream = NOT_SET;
		if (m_running) {
			shutdown();
			join();
		}
	} else {
		LOG_WARN(BulkIOToSDDSProcessor, "Was told to remove stream that was not already set.");
	}
}


void BulkIOToSDDSProcessor::removeShortStream(bulkio::InShortStream stream) {
	LOG_INFO(BulkIOToSDDSProcessor, "Removing stream: " << stream.streamID());
	boost::unique_lock<boost::mutex> lock(m_streamLock);
	if (m_activeStream == SHORT_STREAM && m_shortStream.streamID() == stream.streamID()) {
		m_activeStream = NOT_SET;
		if (m_running) {
			shutdown();
			join();
		}
	} else {
		LOG_WARN(BulkIOToSDDSProcessor, "Was told to remove stream that was not already set.");
	}
}

void BulkIOToSDDSProcessor::removeOctetStream(bulkio::InOctetStream stream) {
	LOG_INFO(BulkIOToSDDSProcessor, "Removing stream: " << stream.streamID());
	boost::unique_lock<boost::mutex> lock(m_streamLock);
	if (m_activeStream == OCTET_STREAM && m_octetStream.streamID() == stream.streamID()) {
		m_activeStream = NOT_SET;
		if (m_running) {
			shutdown();
			join();
		}
	} else {
		LOG_WARN(BulkIOToSDDSProcessor, "Was told to remove stream that was not already set.");
	}
}
