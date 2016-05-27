/*
 * BulkIOToSDDSProcessor.cpp
 *
 *  Created on: May 26, 2016
 *      Author: ylbagou
 */

#include "BulkIOToSDDSProcessor.h"

PREPARE_LOGGING(BulkIOToSDDSProcessor)

BulkIOToSDDSProcessor::BulkIOToSDDSProcessor(): m_shutdown(false), m_streamSet(false), m_running(false), m_processorThread(NULL) {
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

	if (m_streamSet) {
		m_processorThread = new boost::thread(boost::bind(&BulkIOToSDDSProcessor::_run, this));
	}
}

void BulkIOToSDDSProcessor::_run() {
	m_running = true;
	while (not m_shutdown) {
		bulkio::FloatDataBlock block = m_floatStream.read(1024);
		std::cout << "Got " << block.size() << " floats!" << std::endl;
	}
	m_shutdown = false;
	m_running = false;
}

/**
 * If the stream isn't already set this will set the stream
 */
void BulkIOToSDDSProcessor::setStream(bulkio::InFloatStream stream) {
	if (m_streamSet) {
		LOG_ERROR(BulkIOToSDDSProcessor, "There is already an active stream named: " << m_floatStream.streamID() << ", cannot set new stream: " << stream.streamID());
		//TODO: Exception?
		return;
	}

	m_floatStream = stream;
	m_streamSet = true;

}

/**
 * Since only a single stream is allowed this just sets
 * a boolean to say that the stream is not set if the
 * provided stream's streamID matches the current set stream.
 */
void BulkIOToSDDSProcessor::removeStream(bulkio::InFloatStream stream) {
	if (m_streamSet && m_floatStream.streamID() == stream.streamID()) {
		m_streamSet = false;
	} else {
		LOG_WARN(BulkIOToSDDSProcessor, "Was told to remove stream that was not already set.");
	}
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
