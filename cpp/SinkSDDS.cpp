/**************************************************************************

    This is the component code. This file contains the child class where
    custom functionality can be added to the component. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "SinkSDDS.h"

PREPARE_LOGGING(SinkSDDS_i)

SinkSDDS_i::SinkSDDS_i(const char *uuid, const char *label) :
    SinkSDDS_base(uuid, label)
{
	dataFloatIn->addStreamListener(this, &SinkSDDS_i::streamAdded);
	dataFloatIn->removeStreamListener(this, &SinkSDDS_i::streamRemoved);
}

SinkSDDS_i::~SinkSDDS_i(){}

void SinkSDDS_i::constructor(){}

void SinkSDDS_i::start() throw (CORBA::SystemException, CF::Resource::StartError) {
	if (started()) {
		LOG_WARN(SinkSDDS_i, "Already started, call to start ignored.");
		return;
	}

	m_processor.run();

	// Call the parent start
	SinkSDDS_base::start();
}

void SinkSDDS_i::stop () throw (CF::Resource::StopError, CORBA::SystemException) {
	m_processor.shutdown(); // Tells the read thread to shutdown on next pass.
	SinkSDDS_base::stop(); // Opens the port up so that the stream object will return and free up the read lock.
	m_processor.join(); // Joins the processing thread
}
void SinkSDDS_i::streamAdded(bulkio::InFloatStream stream)
{
	// TODO: Lock needed, could be called multiple times from multiple threads.
	LOG_DEBUG(SinkSDDS_i, "Received new float stream: " << stream.streamID());
	m_processor.setStream(stream);
	if (started()) {
		m_processor.run();
	}
}

void SinkSDDS_i::streamRemoved(bulkio::InFloatStream stream) {
	// TODO: Lock needed, could be called multiple times from multiple threads.
	LOG_DEBUG(SinkSDDS_i, "Removing float stream: " << stream.streamID());
	m_processor.removeStream(stream);
}

int SinkSDDS_i::serviceFunction()
{
    LOG_DEBUG(SinkSDDS_i, "No service function in SinkSDDS component, returning FINISH");
    return FINISH;
}
