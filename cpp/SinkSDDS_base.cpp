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
#include "SinkSDDS_base.h"

/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY

    The following class functions are for the base class for the component class. To
    customize any of these functions, do not modify them here. Instead, overload them
    on the child class

******************************************************************************************/

SinkSDDS_base::SinkSDDS_base(const char *uuid, const char *label) :
    Component(uuid, label),
    ThreadedComponent()
{
    loadProperties();

    dataShortIn = new bulkio::InShortPort("dataShortIn");
    addPort("dataShortIn", dataShortIn);
    dataFloatIn = new bulkio::InFloatPort("dataFloatIn");
    addPort("dataFloatIn", dataFloatIn);
    dataOctetIn = new bulkio::InOctetPort("dataOctetIn");
    addPort("dataOctetIn", dataOctetIn);
    dataSddsOut = new bulkio::OutSDDSPort("dataSddsOut");
    addPort("dataSddsOut", dataSddsOut);
}

SinkSDDS_base::~SinkSDDS_base()
{
    delete dataShortIn;
    dataShortIn = 0;
    delete dataFloatIn;
    dataFloatIn = 0;
    delete dataOctetIn;
    dataOctetIn = 0;
    delete dataSddsOut;
    dataSddsOut = 0;
}

/*******************************************************************************************
    Framework-level functions
    These functions are generally called by the framework to perform housekeeping.
*******************************************************************************************/
void SinkSDDS_base::start() throw (CORBA::SystemException, CF::Resource::StartError)
{
    Component::start();
    ThreadedComponent::startThread();
}

void SinkSDDS_base::stop() throw (CORBA::SystemException, CF::Resource::StopError)
{
    Component::stop();
    if (!ThreadedComponent::stopThread()) {
        throw CF::Resource::StopError(CF::CF_NOTSET, "Processing thread did not die");
    }
}

void SinkSDDS_base::releaseObject() throw (CORBA::SystemException, CF::LifeCycle::ReleaseError)
{
    // This function clears the component running condition so main shuts down everything
    try {
        stop();
    } catch (CF::Resource::StopError& ex) {
        // TODO - this should probably be logged instead of ignored
    }

    Component::releaseObject();
}

void SinkSDDS_base::loadProperties()
{
    addProperty(network_settings,
                network_settings_struct(),
                "network_settings",
                "",
                "readwrite",
                "",
                "external",
                "property");

    addProperty(sdds_settings,
                sdds_settings_struct(),
                "sdds_settings",
                "",
                "readwrite",
                "",
                "external",
                "property");

    addProperty(sdds_attach_settings,
                sdds_attach_settings_struct(),
                "sdds_attach_settings",
                "sdds_attach_settings",
                "readwrite",
                "",
                "external",
                "property");

    addProperty(override_sdds_header,
                override_sdds_header_struct(),
                "override_sdds_header",
                "override_sdds_header",
                "readwrite",
                "",
                "external",
                "property");

    addProperty(status,
                status_struct(),
                "status",
                "status",
                "readonly",
                "",
                "external",
                "property");

}


