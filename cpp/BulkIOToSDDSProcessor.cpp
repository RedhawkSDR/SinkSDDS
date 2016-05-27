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
			bytes_read = m_floatBlock.size() / sizeof(float);
			*dataPointer = (char *) m_floatBlock.data();
		}
		break;
	case SHORT_STREAM:
		m_shortBlock = m_shortStream.read(SDDS_DATA_SIZE / sizeof(short));
		if (!!m_shortBlock) {
			bytes_read = m_shortBlock.size() / sizeof(short);
			*dataPointer = (char *) m_shortBlock.data();
		}
		break;
	case OCTET_STREAM:
		m_octetBlock = m_octetStream.read(SDDS_DATA_SIZE / sizeof(char));
		if (!!m_octetBlock) {
			bytes_read = m_octetBlock.size() / sizeof(char);
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
		bytes_read = getDataPointer(&sddsDataBlock);
		if (not bytes_read) {
			//TODO: LOG about EOS
			m_activeStream = NOT_SET;
			m_shutdown = true;
			continue;
		}
		std::cout << "Got " << bytes_read << " bytes" << std::endl;
	}
	m_shutdown = false;
	m_running = false;
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
