#ifndef SINKSDDS_I_IMPL_H
#define SINKSDDS_I_IMPL_H

#include "SinkSDDS_base.h"
#include "BulkIOToSDDSProcessor.h"
#include <netinet/ip.h>
#include "socketUtils/multicast.h"
#include "socketUtils/unicast.h"

class SinkSDDS_i : public SinkSDDS_base
{
    ENABLE_LOGGING
    public:
        SinkSDDS_i(const char *uuid, const char *label);
        ~SinkSDDS_i();
        void constructor();
        int serviceFunction();
        void start() throw (CORBA::SystemException, CF::Resource::StartError);
        void stop () throw (CF::Resource::StopError, CORBA::SystemException);

    private:
        void floatStreamAdded(bulkio::InFloatStream stream);
        void floatStreamRemoved(bulkio::InFloatStream stream);

        void shortStreamAdded(bulkio::InShortStream stream);
		void shortStreamRemoved(bulkio::InShortStream stream);

		void octetStreamAdded(bulkio::InOctetStream stream);
		void octetStreamRemoved(bulkio::InOctetStream stream);

        BulkIOToSDDSProcessor m_processor;
        int setupSocket();
        connection_t m_connection;
};

#endif // SINKSDDS_I_IMPL_H
