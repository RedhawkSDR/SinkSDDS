#ifndef SINKSDDS_I_IMPL_H
#define SINKSDDS_I_IMPL_H

#include "SinkSDDS_base.h"
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
        void streamAdded(bulkio::InFloatStream stream);
        void streamRemoved(bulkio::InFloatStream stream);
        BulkIOToSDDSProcessor m_processor;
};

#endif // SINKSDDS_I_IMPL_H
