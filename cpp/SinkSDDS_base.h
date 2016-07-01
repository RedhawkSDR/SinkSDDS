#ifndef SINKSDDS_BASE_IMPL_BASE_H
#define SINKSDDS_BASE_IMPL_BASE_H

#include <boost/thread.hpp>
#include <ossie/Component.h>
#include <ossie/ThreadedComponent.h>

#include <bulkio/bulkio.h>
#include "struct_props.h"

class SinkSDDS_base : public Component, protected ThreadedComponent
{
    public:
        SinkSDDS_base(const char *uuid, const char *label);
        ~SinkSDDS_base();

        void start() throw (CF::Resource::StartError, CORBA::SystemException);

        void stop() throw (CF::Resource::StopError, CORBA::SystemException);

        void releaseObject() throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);

        void loadProperties();

    protected:
        // Member variables exposed as properties
        /// Property: network_settings
        network_settings_struct network_settings;
        /// Property: sdds_settings
        sdds_settings_struct sdds_settings;
        /// Property: sdds_attach_settings
        sdds_attach_settings_struct sdds_attach_settings;
        /// Property: override_sdds_header
        override_sdds_header_struct override_sdds_header;
        /// Property: status
        status_struct status;

        // Ports
        /// Port: dataShortIn
        bulkio::InShortPort *dataShortIn;
        /// Port: dataFloatIn
        bulkio::InFloatPort *dataFloatIn;
        /// Port: dataOctetIn
        bulkio::InOctetPort *dataOctetIn;
        /// Port: dataSddsOut
        bulkio::OutSDDSPort *dataSddsOut;

    private:
};
#endif // SINKSDDS_BASE_IMPL_BASE_H
