# rh.SinkSDDS

## Table of Contents

* [Description](#description)
* [Design](#design)
* [Properties](#properties)
* [SRI Keywords](#sri-keywords)
* [Known Issues](#known-issuesi)

## Description

The rh.SinkSDDS component will take in a single BulkIO stream on one of the three input ports and serve a single SDDS stream over the provided multicast or unicast address. The component is currently limited to a single stream. The component will perform a BulkIO attach call on any existing connections at start and will call attach on any dynamically made connections during runtime. BulkIO SRI is used to set the SDDS header information unless overridden via properties and the SRI is passed across the SDDS BulkIO connection to any downstream components. See the [Properties](#properties) section for information on overriding SDDS header values and the [SRI Keywords](#sri-keywords) section for information on KEYWORDs created.

## Design

The code is divided into two main classes, the component class and the templated BulkIOToSDDS processor class. The component class contains three instances of the processor class, one for each port type; float, short, and octet.

The component class is responsible for the following actions:
 - Stream listeners - The component class registers new stream listeners and remove stream listeners for each port so that it can provide the appropriate processor with the new stream. It is also the only entity aware of all three bulkIO to SDDS processors so it ensures that only a single stream is active when new streams come in.
 - Property callbacks - Using the setPropertyConfigureImpl API, the Component class intercepts the REDHAWK configure call for the struct properties and ensures that the component is not running when trying to set these properties.
 - New Connection Listener - When a new dynamic connection is made after the component is already running, the SRI and BulkIO Attach call need to be made, the component class informs the processor classes of new dynamic connections so this may occur.
 - Socket creation - The component class is responsible for creating the unicast or multicast socket connection and throwing an appropriate exception if the socket cannot be opened. A successful socket creation will then be handed down to the processor classes to reference.
 - Start/Stop API - The component class overrides the start / stop REDHAWK calls so that it can create the socket and start the appropriate processors. During stop, it will cleanup the processor threads and close the socket. The traditional service function / process thread is not used by this component.

The BulkIOToSDDSProcessor class is a templated class so that it can handle any of the current port type. It is responsible for the following actions:
 - Pulling data from BulkIO - Using the stream API, the processor will attempt to pull exactly 1024 bytes, the SDDS payload size, from the BulkIO stream. This may not always be the case due to end of streams, SRI changes etc and is discussed in more depth below.
 - Setting SDDS header values - Fields such as, but not limited to the SDDS timing, frequency, and complex field are derived from the BulkIO SRI. Optionally a user may override these fields. 
 - Pushing SDDS Packets - Using the socket connection passed in from the Component class, the processor class will push SDDS packets down to the kernel using the sendmsg API and the scatter / gather approach. The scatter / gather approach allows the packet to be divided into three separate arrays
   - SDDS Header Template - The class has a single SDDS header that is updated for each packet
   - SDDS Payload - Read from BulkIO this is usually 1024 however can be less on an EOS of SRI change
   - Zero Padding - Generally not used but in cases where the SDDS Payload is less than 1024 the packet is padded

## Properties

Properties and their descriptions are below, struct props are shown with their struct properties in a table below:

**network_settings** - Settings for the network connection.

| Struct Property      | Description  |
| ------------- | -----|
| interface | The network interface to bind to.  Do not include the VLAN in the interface name. (eg. For eth0.28 the interface should be set to "eth0" NOT "eth0.28") |
| ip_address | unicast/multicast address or group ID to publish data to. |
| port | UDP port used to publish data. (default SDDS port is: 29495)  |
| vlan | UDP port used to publish data. |

**sdds_settings** - Settings related to standard fields in the SDDS Packet or data portion which cannot be derived from BulkIO metadata.

| Struct Property      | Description  |
| ------------- | -----|
| standard_format | The SF (Standard Format) field is used to identify whether or not the packet conforms to the SDDS standard. For SDDS standard packets, the SF bit shall be set to a value of 1. The SF bit shall be set to a value of zero for non-standard packets. |
| original_format | The OF (Original Format) field identifies the original format of the data transmitted. If the data was originally offset binary and has been converted to 2's complement, the OF value is set to one. Otherwise, the data is 2's complement and has not been converted and the OF value is set to zero. |
| spectral_sense | The SS (Spectral Sense) field identifiees whether or not the spectral sense has been inverted from the original input. The SS value is set to one if the spectral sense has been inverted. The SS value is set to zero if the spectral sense has not been inverted.  |
| endian_representation | The endianness (Big or Little) of the data portion of the SDDS packet. Defaults to Network Byte Order (Big Endian). This will also affect the SRI keyword DATA_REF_STR and set it appropriately |

**sdds_attach_settings** - Settings related to the BulkIO based SDDS Attach and detach API.

| Struct Property      | Description  |
| ------------- | -----|
| time_tag_valid | Used only in the attach call. The attach call is made prior to the bulkIO TimeStamp being available so the true BULKIO::TCS_VALID flag cannot be checked. This is only used during the call to attach. |
| user_id | Used as a parameter to the attach call. |
| downstream_give_sri_priority | Informs downstream components, via the BULKIO_SRI_PRIORITY keyword, to override the xdelta and real/complex mode found in the SDDS Packet header in place of the xdelta and mode found in the supplied SRI.  |

**override_sdds_header** - Used to optionally override values in the SDDS header which would otherwise be derived from BulkIO metadata or set to a reasonable value.

| Struct Property      | Description  |
| ------------- | -----|
| enabled | If true, the values in this struct will override the default values used by this component. Each property should have a description which explains what values would otherwise be used. |
| dmode | The data mode field identifies the structure of the data field. See the full specification for details. Unless overridden, this is set to 1 for byte samples, 2 for shorts, and 0 for floats as floats are not part of the standadr. |
| bps | The number of bits per sample where the sample size in this case is only refering to the size of the native type used eg. byte, short, float. Bytes are 8, Shorts 16, and since only 5 bits are available, a float is labeled as 31 rather than 32.  Unless overridden, this is derived from the sizeof call on the native bulkIO packet type. |
| cx | Denotes if the data portion of the packet represents real (0) or complex (1) values. Unless overridden, this is derived from the mode field within the SRI. |
| msv | Denotes if the samples within this packet span a 1-millisecond boundry. Unless overridden, this field is not used and set to zero. |
| ttv | Time Tag Valid field denotes if the values stored within the Time Tag information fields (Time Tag, Time Tag Extension) are valid.  Unless overridden, this is derived from the tcstatus field of BulkIO Timestamps and is set to true if equal to TCS_VALID and false otherwise. |
| sscv | Synchronous Sample Clock Valid field is 1 if the SSC information fields (df/dt and Frequency) are valid and zero otherwise. Unless overridden, this is always set to 1. |
| msptr | Points to the first sample in the data field that occurred after the 1-millisecond event. Unless overridden, this is always set to 0. |
| msdel | The 1-ms Delta is the time difference between the 1-millisecond event and the first positive going transistion of the SSC that occurred after the 1-millisecond event. Unless overridden, this is always set to 0. |
| frequency | The frequency field contains the frequency of the SSC. This value represents the instantaneous frqeuency of the SSC associated with the first sample of the frame.  Unless overridden, this is derived from the xdelta found in the SRI. |
| dfdt | The df/dt field measures the rate at which the frequency is changing. The value represents the delta between the instantaneous frequency of the last SSC of the packet and the instantaneous frequency of the first SSC of the packet divided by the packet duration. Unless overridden, this is set to 0.0 |

## SRI Keywords

The SinkSDDS component does not check for or react to any specific keywords. Any keywords which exist in the given SRI are forwarded to downstream components unless they are the same keywords written to by SinkSDDS in which case SinkSDDS will override the values.

* **BULKIO_SRI_PRIORITY** - If the sdds_attach_settings::downstream_give_sri_priority property is set to true, the BULKIO_SRI_PRIORITY keyword will be set with a value of one. If the sdds_attach_settings::downstream_give_sri_priority property is set to false, the BULKIO_SRI_PRIORITY keyword will not be added.
* **DATA_REF_STR** - If the sdds_settings::endian_representation property is set to LITTLE_ENDIAN (0) the DATA_REF_STR keyword will be set to 43981 (0xABCD) while if the sdds_settings::endian_representation property is set to BIG_ENDIAN (1) the DATA_REF_STR keyword will be set to 52651 (0xCDAB)

## Known Issues

* Calculation of dfdt field - Currently the df/dt field in the SDDS header defaults to zero unless overridden by the user. The df/dt field is supposed to represents the change in sample clock frequency in units of Hz/sec between the first and last sample in the SDDS packet. This exact information is not found in BulkIO so a direct mapping is not possible however BulkIO does allow you to check if multiple timestamps are available and some estimation based on that may be possible.
* Change of BulkIO Mode - Changing the BulkIO mode field mid-stream is not recommended, a user should send an EOS and start a new stream with the changed mode. The component will gracefully deal with a mode change though with the following outcomes. Going from Real->Complex this will cause 1 SDDS packets worth of data to have an incorrect time stamp. Going from Complex->Real will cause a single packet to be erroneously padded with zeros.
