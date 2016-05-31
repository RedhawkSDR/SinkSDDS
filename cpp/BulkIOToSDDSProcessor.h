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
#include "sddspacket.h"
#include "socketUtils/multicast.h"
#include "socketUtils/unicast.h"
#include "struct_props.h"

enum ActiveStream {NOT_SET, FLOAT_STREAM, SHORT_STREAM, OCTET_STREAM};

#define SDDS_DATA_SIZE 1024
#define SDDS_HEADER_SIZE 56
#define SDDS_FLOAT_BPS 31
#define SDDS_SHORT_BPS 16
#define SDDS_OCTET_BPS 8

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

	void setConnection(connection_t connection);
	void run();
	void shutdown();
	void join();
private:
	bulkio::InFloatStream m_floatStream;
	bulkio::InShortStream m_shortStream;
	bulkio::InOctetStream m_octetStream;
	size_t getDataPointer(char **dataPointer);
	int sendPacket(char* sddsData);
	void initializeSDDSHeader();
	void setSddsSettings(sdds_settings_struct settings);

	void _run();
	bool m_shutdown, m_running;
	boost::thread *m_processorThread;
	ActiveStream m_activeStream;
	boost::mutex m_streamLock;
	Resource_impl *m_parent;

	bulkio::FloatDataBlock m_floatBlock;
	bulkio::ShortDataBlock m_shortBlock;
	bulkio::OctetDataBlock m_octetBlock;
	connection_t m_connection;
	msghdr m_pkt_template;
	SDDSpacket m_sdds_template;
	iovec m_msg_iov[2];


};

#endif /* BULKIOTOSDDSPROCESSOR_H_ */
