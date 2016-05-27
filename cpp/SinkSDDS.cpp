/**************************************************************************

    This is the component code. This file contains the child class where
    custom functionality can be added to the component. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "SinkSDDS.h"

PREPARE_LOGGING(SinkSDDS_i)

SinkSDDS_i::SinkSDDS_i(const char *uuid, const char *label) :
    SinkSDDS_base(uuid, label),
	m_processor(this)
{
	dataFloatIn->addStreamListener(this, &SinkSDDS_i::floatStreamAdded);
	dataFloatIn->removeStreamListener(this, &SinkSDDS_i::floatStreamRemoved);

	dataShortIn->addStreamListener(this, &SinkSDDS_i::shortStreamAdded);
	dataShortIn->removeStreamListener(this, &SinkSDDS_i::shortStreamRemoved);

	dataOctetIn->addStreamListener(this, &SinkSDDS_i::octetStreamAdded);
	dataOctetIn->removeStreamListener(this, &SinkSDDS_i::octetStreamRemoved);
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

void SinkSDDS_i::floatStreamAdded(bulkio::InFloatStream stream) { m_processor.setFloatStream(stream); }
void SinkSDDS_i::floatStreamRemoved(bulkio::InFloatStream stream) {	m_processor.removeFloatStream(stream);}

void SinkSDDS_i::shortStreamAdded(bulkio::InShortStream stream) { m_processor.setShortStream(stream); }
void SinkSDDS_i::shortStreamRemoved(bulkio::InShortStream stream) {	m_processor.removeShortStream(stream);}

void SinkSDDS_i::octetStreamAdded(bulkio::InOctetStream stream) { m_processor.setOctetStream(stream); }
void SinkSDDS_i::octetStreamRemoved(bulkio::InOctetStream stream) {	m_processor.removeOctetStream(stream);}

int SinkSDDS_i::serviceFunction()
{
    LOG_DEBUG(SinkSDDS_i, "No service function in SinkSDDS component, returning FINISH");
    return FINISH;
}
