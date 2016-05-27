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
#include <Resource_impl.h>

enum ActiveStream {NOT_SET, FLOAT_STREAM, SHORT_STREAM, OCTET_STREAM};

#define SDDS_DATA_SIZE 1024

class BulkIOToSDDSProcessor {
	ENABLE_LOGGING
public:
	BulkIOToSDDSProcessor(Resource_impl *parent);
	virtual ~BulkIOToSDDSProcessor();

	void setFloatStream(bulkio::InFloatStream stream);
	void setShortStream(bulkio::InShortStream stream);
	void setOctetStream(bulkio::InOctetStream stream);

	void removeFloatStream(bulkio::InFloatStream stream);
	void removeShortStream(bulkio::InShortStream stream);
	void removeOctetStream(bulkio::InOctetStream stream);

	void run();
	void shutdown();
	void join();
private:
	bulkio::InFloatStream m_floatStream;
	bulkio::InShortStream m_shortStream;
	bulkio::InOctetStream m_octetStream;
	size_t getDataPointer(char **dataPointer);

	void _run();
	bool m_shutdown, m_running;
	boost::thread *m_processorThread;
	ActiveStream m_activeStream;
	boost::mutex m_streamLock;
	Resource_impl *m_parent;

	bulkio::FloatDataBlock m_floatBlock;
	bulkio::ShortDataBlock m_shortBlock;
	bulkio::OctetDataBlock m_octetBlock;


};

#endif /* BULKIOTOSDDSPROCESSOR_H_ */
