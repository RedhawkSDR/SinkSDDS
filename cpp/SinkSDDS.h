/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of REDHAWK rh.SinkSDDS.
 *
 * REDHAWK rh.SinkSDDS is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * REDHAWK rh.SinkSDDS is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 */
#ifndef SINKSDDS_I_IMPL_H
#define SINKSDDS_I_IMPL_H

#include "SinkSDDS_base.h"
#include "BulkIOToSDDSProcessor.h"
#include <netinet/ip.h>
#include "socketUtils/multicast.h"
#include "socketUtils/unicast.h"
#include "BulkIOToSDDSProcessor.h"
#include "StreamsDoneCallBackInterface.h"
#include <vector>



class SinkSDDS_i : public SinkSDDS_base, public StreamsDoneCallBackInterface
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
        struct status_struct get_status_struct();

        void setFloatStream(bulkio::InFloatStream floatStream);
        void setShortStream(bulkio::InShortStream shortStream);
        void setOctetStream(bulkio::InOctetStream octetStream);
        void newConnectionMade(const char* connectionId);

        void streamsDone(std::string streamId);

        std::vector<bulkio::InFloatStream> m_floatStreamOnDeck;
        std::vector<bulkio::InShortStream> m_shortStreamOnDeck;
        std::vector<bulkio::InOctetStream> m_octetStreamOnDeck;

    private:
        int setupSocket();
        bool streamIsOnDeck();
        bool streamIsActive();

        void dropOnDeckStream();
        connection_t m_connection;
        boost::mutex m_stream_lock;

        BulkIOToSDDSProcessor<bulkio::InShortStream> m_shortproc;
        BulkIOToSDDSProcessor<bulkio::InFloatStream> m_floatproc;
        BulkIOToSDDSProcessor<bulkio::InOctetStream> m_octetproc;


};

#endif // SINKSDDS_I_IMPL_H
