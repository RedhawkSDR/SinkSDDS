#ifndef SINKSDDS_I_IMPL_H
#define SINKSDDS_I_IMPL_H

#include "SinkSDDS_base.h"
#include "BulkIOToSDDSProcessor.h"
#include <netinet/ip.h>
#include "socketUtils/multicast.h"
#include "socketUtils/unicast.h"
#include "BulkIOToSDDSProcessor.h"

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
        int setupSocket();
        connection_t m_connection;

        BulkIOToSDDSProcessor<bulkio::InShortStream> m_shortproc;
        BulkIOToSDDSProcessor<bulkio::InFloatStream> m_floatproc;
        BulkIOToSDDSProcessor<bulkio::InOctetStream> m_octetproc;


};

#endif // SINKSDDS_I_IMPL_H
