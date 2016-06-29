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
        void set_sdds_settings_struct(struct sdds_settings_struct request);
        void set_network_settings_struct(struct network_settings_struct request);
        void set_override_sdds_header_struct(struct override_sdds_header_struct request);

        void setFloatStream(bulkio::InFloatStream floatStream);
        void setShortStream(bulkio::InShortStream shortStream);
        void setOctetStream(bulkio::InOctetStream octetStream);
        void newConnectionMade(const char* connectionId);

    private:
        int setupSocket();
        size_t getNumberOfActiveStreams();
        connection_t m_connection;
        boost::mutex m_new_stream_lock;

        BulkIOToSDDSProcessor<bulkio::InShortStream> m_shortproc;
        BulkIOToSDDSProcessor<bulkio::InFloatStream> m_floatproc;
        BulkIOToSDDSProcessor<bulkio::InOctetStream> m_octetproc;


};

#endif // SINKSDDS_I_IMPL_H
