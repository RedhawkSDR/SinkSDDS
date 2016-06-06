/*
 * BulkIOToSDDSProcessor.cpp
 *
 *  Created on: May 26, 2016
 *      Author: ylbagou
 */

#include "BulkIOToSDDSProcessor.h"

PREPARE_LOGGING(BulkIOToSDDSProcessor)

BulkIOToSDDSProcessor::BulkIOToSDDSProcessor(Resource_impl *parent):
m_shutdown(false), m_running(false), m_processorThread(NULL), m_activeStream(NOT_SET), m_parent(parent), m_first_run(true), m_seq(0) {

	m_pkt_template.msg_name = NULL;
	m_pkt_template.msg_namelen = 0;
	m_pkt_template.msg_iov = &m_msg_iov[0];
	m_pkt_template.msg_iovlen = 3; // We use three iov. One is the SDDS header and the second is the payload, the third for any zero padding if necessary.
	m_pkt_template.msg_control = NULL;
	m_pkt_template.msg_controllen = 0;
	m_pkt_template.msg_flags = NULL;

	m_msg_iov[0].iov_base = &m_sdds_template;
	m_msg_iov[0].iov_len = SDDS_HEADER_SIZE;
	m_msg_iov[1].iov_base = NULL; // This will be set later.
	m_msg_iov[1].iov_len = SDDS_DATA_SIZE;
	m_msg_iov[2].iov_base = m_zero_pad_buffer;
	m_msg_iov[2].iov_len = 0;

	memset(&m_zero_pad_buffer, 0, SDDS_DATA_SIZE);

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
	LOG_TRACE(BulkIOToSDDSProcessor, "Entering the Run Method");
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

	m_processorThread = new boost::thread(boost::bind(&BulkIOToSDDSProcessor::_run, boost::ref(*this)));
	LOG_TRACE(BulkIOToSDDSProcessor, "Leaving the Run Method");
}

//TODO: SOOOOO MUCH COPY PASTE!
size_t BulkIOToSDDSProcessor::getDataPointer(char **dataPointer, bool &sriChanged) {
	size_t bytes_read = 0;

	switch (m_activeStream) {
	case FLOAT_STREAM:
		m_floatBlock = m_floatStream.read(SDDS_DATA_SIZE / sizeof(float));
		if (!!m_floatBlock) {  //TODO: Document bang bang
			bytes_read = m_floatBlock.size() * sizeof(float);
			*dataPointer = reinterpret_cast<char*>(m_floatBlock.data());
			if (m_first_run || m_floatBlock.sriChanged()) {
				m_sri = m_floatBlock.sri();
				sriChanged = true;
			}
		}
		break;
	case SHORT_STREAM:
		m_shortBlock = m_shortStream.read(SDDS_DATA_SIZE / sizeof(short));
		if (!!m_shortBlock) {
			bytes_read = m_shortBlock.size() * sizeof(short);
			*dataPointer = reinterpret_cast<char*>(m_shortBlock.data());
			if (m_first_run || m_shortBlock.sriChanged()) {
				m_sri = m_shortBlock.sri();
				sriChanged = true;
			}
		}
		break;
	case OCTET_STREAM:
		m_octetBlock = m_octetStream.read(SDDS_DATA_SIZE / sizeof(char));
		if (!!m_octetBlock) {
			bytes_read = m_octetBlock.size() * sizeof(char);
			*dataPointer = reinterpret_cast<char*>(m_octetBlock.data());
			if (m_first_run || m_octetBlock.sriChanged()) {
				m_sri = m_octetBlock.sri();
				sriChanged = true;
			}
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

void BulkIOToSDDSProcessor::setSddsHeaderFromSri() {
	m_sdds_template.cx = m_sri.mode;
	m_sdds_template.set_freq(1.0 / m_sri.xdelta);
	if (m_first_run) {
		m_sdds_template.sos = 1;
	}
//	m_sdds_template.dFdT // TODO: What should we do with this?
}
void BulkIOToSDDSProcessor::_run() {
	LOG_TRACE(BulkIOToSDDSProcessor, "Entering the _run Method");
	m_running = true;
	char *sddsDataBlock;
	int bytes_read = 0;

	while (not m_shutdown) {
		bool sriChanged = false;
		bytes_read = getDataPointer(&sddsDataBlock, sriChanged);
		LOG_TRACE(BulkIOToSDDSProcessor, "Received " << bytes_read << " bytes from bulkIO");
		if (sriChanged) {
//			pushSri(); // TODO:
			setSddsHeaderFromSri();
		}


		if (not bytes_read) {
			//TODO: LOG and send an EOS
			m_activeStream = NOT_SET;
			m_shutdown = true;
			continue;
		}

		if (sendPacket(sddsDataBlock, bytes_read) < 0) {
			LOG_ERROR(BulkIOToSDDSProcessor, "Failed to push packet over socket, stream will be closed.");
			m_activeStream = NOT_SET;
			m_shutdown = true;
			continue;
		}

		m_first_run = false; // TODO: Can we not set this every single time we run this loop? Seems silly.
	}

	m_first_run = true;
	m_shutdown = false;
	m_running = false;
	LOG_TRACE(BulkIOToSDDSProcessor, "Exiting the _run Method");
}


int BulkIOToSDDSProcessor::sendPacket(char* dataBlock, int num_bytes) {
/**
 * Four different situations covered here.
 * 1. We've read exactly 1024 bytes. Occurs when the mode is real, we get exactly one packets worth and this method runs only once.
 * 2. We've read exactly 2*1024 bytes. Occurs when the mode is complex, this method runs twice recursively. Decreasing the num_bytes
 * 3. We've read less less than 1024 bytes, occurs when the stream is changing.
 * 4. We've read more than 1024, but less than 2*1024.
 */

	LOG_TRACE(BulkIOToSDDSProcessor, "sendPacket called, told to send " << num_bytes <<  "bytes");
	if (num_bytes <= 0)
		return 0;

	ssize_t numSent;
	// Reset the start of sequence flag if it is not the first packet sent, the sequence number has rolled over, and sos is currently set.
	if (not m_first_run && m_seq == 0 && m_sdds_template.sos) { m_sdds_template.sos = 0; }

	m_sdds_template.set_seq(m_seq);
	m_seq++;
	if (m_seq != 0 && m_seq % 32 == 31) { m_seq++; } // Account for the parity packet that we aren't sending


	m_msg_iov[1].iov_base = dataBlock;
	m_msg_iov[1].iov_len = (num_bytes >= SDDS_DATA_SIZE) ? SDDS_DATA_SIZE : num_bytes;

	m_msg_iov[2].iov_len = SDDS_DATA_SIZE - m_msg_iov[1].iov_len;

	numSent = sendmsg(m_connection.sock, &m_pkt_template, 0);
	if (numSent < 0) {return numSent;} // Error occurred

	LOG_TRACE(BulkIOToSDDSProcessor, "Pushed " << numSent << " bytes out of socket.")

	// Its possible we read more than a single packet (if the mode is real we would read 2).
	return sendPacket(dataBlock + SDDS_DATA_SIZE, num_bytes - m_msg_iov[1].iov_len);
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
