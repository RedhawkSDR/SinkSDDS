/*
 * BulkIOToSDDSProcessor.h
 *
 *  Created on: May 26, 2016
 *      Author: ylbagou
 */

#ifndef BULKIOTOSDDSPROCESSOR_H_
#define BULKIOTOSDDSPROCESSOR_H_

#include <bulkio/bulkio.h>
#include "struct_props.h"
#include <boost/thread.hpp>
#include <ossie/debug.h>

class BulkIOToSDDSProcessor {
	ENABLE_LOGGING
public:
	BulkIOToSDDSProcessor();
	virtual ~BulkIOToSDDSProcessor();
	void setStream(bulkio::InFloatStream stream);
	void run();
	void shutdown();
	void join();
	void removeStream(bulkio::InFloatStream stream);
private:
	bulkio::InFloatStream m_floatStream;
	void _run();
	bool m_shutdown, m_streamSet, m_running;
	boost::thread *m_processorThread;


};

#endif /* BULKIOTOSDDSPROCESSOR_H_ */
