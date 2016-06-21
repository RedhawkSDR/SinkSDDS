#include "BulkIOToSDDSProcessor.h"

template <class STREAM_TYPE>
PREPARE_LOGGING(BulkIOToSDDSProcessor<STREAM_TYPE>)

template <class STREAM_TYPE>
BulkIOToSDDSProcessor<STREAM_TYPE>::BulkIOToSDDSProcessor(Resource_impl *parent, bulkio::OutSDDSPort * dataSddsOut):
m_parent(parent), m_sdds_out_port(dataSddsOut), m_first_run(true), m_processorThread(NULL), m_shutdown(false), m_running(false), m_active_stream(false),  m_vlan(0), m_seq(0) {

	m_pkt_template.msg_name = NULL;
	m_pkt_template.msg_namelen = 0;
	m_pkt_template.msg_iov = &m_msg_iov[0];
	m_pkt_template.msg_iovlen = 3; // We use three iov. One is the SDDS header and the second is the payload, the third for any zero padding if necessary.
	m_pkt_template.msg_control = NULL;
	m_pkt_template.msg_controllen = 0;
	m_pkt_template.msg_flags = NULL;

	m_msg_iov[0].iov_base = &m_sdds_template;
	m_msg_iov[0].iov_len = SDDS_HEADER_SIZE;
	m_msg_iov[1].iov_base = NULL; // This will be set later.
	m_msg_iov[1].iov_len = SDDS_DATA_SIZE;
	m_msg_iov[2].iov_base = m_zero_pad_buffer;
	m_msg_iov[2].iov_len = 0;

	memset(&m_zero_pad_buffer, 0, SDDS_DATA_SIZE);
	initializeSDDSHeader();
}

template <class STREAM_TYPE>
BulkIOToSDDSProcessor<STREAM_TYPE>::~BulkIOToSDDSProcessor() {
	// Lets hope someone released the port lock
	if (m_processorThread) {
		join();
	}
}

template <class STREAM_TYPE>
bool BulkIOToSDDSProcessor<STREAM_TYPE>::isActive() {
	return m_active_stream;
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::join() {
	if (m_processorThread) {
		m_processorThread->join();
		delete(m_processorThread);
		m_processorThread = NULL;
	} else {
		LOG_DEBUG(BulkIOToSDDSProcessor,"Join called but the thread object is null.");
	}
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::setSddsSettings(sdds_settings_struct settings) {
	if (m_running) {
		LOG_WARN(BulkIOToSDDSProcessor,"Cannot override the SDDS Settings while running.");
		return;
	}

	m_sdds_template.sf = settings.standard_format;
	m_sdds_template.of = settings.original_format;
	m_sdds_template.ss = settings.spectral_sense;
	m_user_settings = settings;
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::run() {
	LOG_TRACE(BulkIOToSDDSProcessor,"Entering the Run Method");
	if (m_processorThread) {
		if (m_running) {
			LOG_ERROR(BulkIOToSDDSProcessor,"The BulkIO To SDDS Processor is already running, cannot start a new stream without first receiving an EOS!");
			return;
		} else {
			join();
		}
	}

	if (not m_active_stream) {
		return;
	}

	callAttach();
	overrideSddsHeader();

	m_processorThread = new boost::thread(boost::bind(&BulkIOToSDDSProcessor<STREAM_TYPE>::_run, boost::ref(*this)));
	LOG_TRACE(BulkIOToSDDSProcessor,"Leaving the Run Method");
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::overrideSddsHeader() {
	if (m_sdds_header_override.enabled) {
		m_sdds_template.bps = m_sdds_header_override.bps;
		m_sdds_template.cx = m_sdds_header_override.cx;
		m_sdds_template.set_dfdt(m_sdds_header_override.dfdt);
		m_sdds_template.dmode = m_sdds_header_override.dmode;
		m_sdds_template.set_freq(m_sdds_header_override.frequency);
		m_sdds_template.set_msdel(m_sdds_header_override.msdel);
		m_sdds_template.set_msptr(m_sdds_header_override.msptr);
		m_sdds_template.set_msv(m_sdds_header_override.msv);
		m_sdds_template.set_sscv(m_sdds_header_override.sscv);
		m_sdds_template.set_ttv(m_sdds_header_override.ttv);
	}
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::callDetach() {
	// There are multiple places were detach can be called during the shutdown process so just return if we've already cleared out the stream.
	if (m_active_stream) {
		LOG_TRACE(BulkIOToSDDSProcessor, "Calling detach on current stream: " << m_stream.streamID());
		m_sdds_out_port->detach(CORBA::string_dup(m_stream.streamID().c_str()));
		m_active_stream = false;
	} else {
		LOG_TRACE(BulkIOToSDDSProcessor, "Was told to call detach but also told there is no active stream!");
	}
}
template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::callAttach(BULKIO::dataSDDS::_ptr_type sdds_input_port) {
	LOG_DEBUG(BulkIOToSDDSProcessor, "Calling attach");
	if (not m_active_stream) {
		LOG_DEBUG(BulkIOToSDDSProcessor, "No active stream so attach will not go through");
		return;
	}

	BULKIO::SDDSStreamDefinition sdef;

	sdef.id = CORBA::string_dup(m_stream.streamID().c_str());

	switch(sizeof(NATIVE_TYPE)) {
	case sizeof(char):
		sdef.dataFormat = (m_stream.sri().mode == 1) ? BULKIO::SDDS_CB : BULKIO::SDDS_SB;
		break;
	case sizeof(short):
		sdef.dataFormat = (m_stream.sri().mode == 1) ? BULKIO::SDDS_CI : BULKIO::SDDS_SI;
		break;
	case sizeof(float):
		sdef.dataFormat = (m_stream.sri().mode == 1) ? BULKIO::SDDS_CF : BULKIO::SDDS_SF;
		break;
	default:
		LOG_ERROR(BulkIOToSDDSProcessor,"Native type size is not what we would expect.");
		break;

	}

	sdef.sampleRate = 1.0/m_stream.sri().xdelta;
	sdef.timeTagValid = m_attach_settings.time_tag_valid;
	sdef.multicastAddress = CORBA::string_dup(inet_ntoa(m_connection.addr.sin_addr));
	sdef.vlan = m_vlan;
	sdef.port = ntohs(m_connection.addr.sin_port);

	if (sdds_input_port == NULL) {
		m_sdds_out_port->attach(sdef, m_attach_settings.user_id.c_str());
	} else {
		sdds_input_port->attach(sdef, m_attach_settings.user_id.c_str());
	}
}
template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::callAttach() {
	callAttach((BULKIO::dataSDDS::_ptr_type)NULL);
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::setConnection(connection_t connection, uint16_t vlan) {
	m_connection = connection;
	m_vlan = vlan;
	// Not sure why but the msg_name and msg_namelen must be set. Perhaps it's a UDP thing. I thought they were optional
	m_pkt_template.msg_name = &m_connection.addr;
	m_pkt_template.msg_namelen = sizeof(m_connection.addr);
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::shutdown() {
	m_shutdown = true;
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::setStream(STREAM_TYPE stream) {
	LOG_INFO(BulkIOToSDDSProcessor,"Received new stream: " << stream.streamID());

	if (m_active_stream) {
		LOG_ERROR(BulkIOToSDDSProcessor,"There is already an active stream named: " << m_stream.streamID() << ", cannot set new stream: " << stream.streamID());
		return;
	}

	m_stream = stream;
	m_active_stream = true;

	if (not m_sdds_header_override.enabled) {
		m_sdds_template.bps = (sizeof(NATIVE_TYPE) == sizeof(float)) ? (31) : 8*sizeof(NATIVE_TYPE);
	}

	if (m_parent->started()) { run(); }
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::removeStream(STREAM_TYPE stream) {
	LOG_INFO(BulkIOToSDDSProcessor,"Removing stream: " << stream.streamID());
	if (m_active_stream && m_stream.streamID() == stream.streamID()) {
		if (m_running) {
			shutdown();
			join();
		}
	} else {
		LOG_WARN(BulkIOToSDDSProcessor,"Was told to remove stream that was not already set.");
	}
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::_run() {
	LOG_TRACE(BulkIOToSDDSProcessor,"Entering the _run Method");
	m_running = true;
	char *sddsDataBlock;
	int bytes_read = 0;

	while (not m_shutdown) {
		bool sriChanged = false;
		bytes_read = getDataPointer(&sddsDataBlock, sriChanged);
		LOG_TRACE(BulkIOToSDDSProcessor,"Received " << bytes_read << " bytes from bulkIO");
		if (sriChanged) {
			LOG_INFO(BulkIOToSDDSProcessor,"Stream SRI has changed, updating SDDS header.");
			pushSri();
			setSddsHeaderFromSri();
		}
		if (not bytes_read) {
			shutdown();
			continue;
		}

		if (sendPacket(sddsDataBlock, bytes_read) < 0) {
			LOG_ERROR(BulkIOToSDDSProcessor,"Failed to push packet over socket, stream will be closed.");
			shutdown();
			continue;
		}

		if (m_stream.eos()) {
			callDetach();
		}
	}

	m_first_run = true;
	m_shutdown = false;
	m_running = false;
	LOG_TRACE(BulkIOToSDDSProcessor,"Exiting the _run Method");
}

template <class STREAM_TYPE>
size_t BulkIOToSDDSProcessor<STREAM_TYPE>::getDataPointer(char **dataPointer, bool &sriChanged) {
	LOG_TRACE(BulkIOToSDDSProcessor,"Entering getDataPointer Method");
	size_t bytes_read = 0;
	size_t complex_scale = (m_stream.sri().mode == 0 ? 1 : 2);

	m_block = m_stream.read(SDDS_DATA_SIZE / sizeof(NATIVE_TYPE) / complex_scale);
	if (!!m_block) {  //Operator overloading for ! (bang) so know if there is no data, however we want to know if there is so we bang bang
		m_current_time = m_block.getTimestamps().front().time;

		bytes_read = m_block.size() * sizeof(NATIVE_TYPE);
		*dataPointer = reinterpret_cast<char*>(m_block.data());
		if (m_user_settings.endian_representation) { // 1 for Big 0 for Little, we assume we're running on a little endian so we byteswap
			uint32_t *buf = reinterpret_cast<uint32_t*>(*dataPointer);

			switch (sizeof(NATIVE_TYPE)) {
			case sizeof(short):
				swab(*dataPointer, *dataPointer, bytes_read);
				break;
			case sizeof(float):
				for (size_t i = 0; i < m_block.size(); ++i) {
					buf[i] = __builtin_bswap32(buf[i]);
				}
				break;
			case sizeof(char):
				// Do nothing
				break;
			default:
				break;
			}

		}
		if (m_first_run || m_block.sriChanged()) {
			m_sri = m_block.sri();
			sriChanged = true;
			if (m_block.sriChangeFlags() & bulkio::sri::MODE) {
				LOG_ERROR(BulkIOToSDDSProcessor,"KNOWN ISSUE!! Mode was changed in the middle of a stream. "
						"Going from Real->Complex this will cause 1 SDDS packets worth of data to have an incorrect time stamp. "
						"Going from Complex->Real will cause a single packet to be erroneously padded with zeros.");
			}
		}
	}

	LOG_TRACE(BulkIOToSDDSProcessor,"Leaving getDataPointer Method");
	return bytes_read;
}

template <class STREAM_TYPE>
int BulkIOToSDDSProcessor<STREAM_TYPE>::sendPacket(char* dataBlock, int num_bytes) {
	/**
	 * Four different situations covered here.
	 * 1. We've read exactly 1024 bytes. Occurs when the mode is real, we get exactly one packets worth and this method runs only once.
	 * 2. We've read exactly 2*1024 bytes. Occurs when the mode is complex, this method runs twice recursively. Decreasing the num_bytes
	 * 3. We've read less less than 1024 bytes, occurs when the stream is changing.
	 * 4. We've read more than 1024, but less than 2*1024.
	 */

	LOG_TRACE(BulkIOToSDDSProcessor,"sendPacket called, told to send " << num_bytes <<  " bytes");
	if (num_bytes <= 0)
		return 0;

	ssize_t numSent;
	// Reset the start of sequence flag if it is not the first packet sent, the sequence number has rolled over, and sos is currently set.
	if (not m_first_run && m_seq == 0 && m_sdds_template.sos) { m_sdds_template.sos = 0; }

	m_sdds_template.set_seq(m_seq);
	m_seq++;
	if (m_seq != 0 && m_seq % 32 == 31) { m_seq++; } // Account for the parity packet that we aren't sending

	m_msg_iov[1].iov_base = dataBlock;
	m_msg_iov[1].iov_len = (num_bytes >= SDDS_DATA_SIZE) ? SDDS_DATA_SIZE : num_bytes;

	m_msg_iov[2].iov_len = SDDS_DATA_SIZE - m_msg_iov[1].iov_len;

	setSddsTimestamp();
	numSent = sendmsg(m_connection.sock, &m_pkt_template, 0);
	if (numSent < 0) {return numSent;} // Error occurred

	LOG_TRACE(BulkIOToSDDSProcessor,"Pushed " << numSent << " bytes out of socket.")

	// Its possible we read more than a single packet if the mode changed on us (known bug)
	return sendPacket(dataBlock + SDDS_DATA_SIZE, num_bytes - m_msg_iov[1].iov_len);
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::initializeSDDSHeader(){
	m_sdds_template.pp = 0;  //PP Parity Packet
	m_sdds_template.set_sscv(1); // Valid Synchronous Sample Clock (frequency & df/dt)
	m_sdds_template.set_dfdt(0.0);

	switch (sizeof(NATIVE_TYPE)) {
	case sizeof(char):
		m_sdds_template.dmode = 1;
		break;
	case sizeof(short):
		m_sdds_template.dmode = 2;
		break;
	default:
		m_sdds_template.dmode = 0;
		break;

	}

	for (size_t i = 0; i < SSD_LENGTH; ++i) { m_sdds_template.ssd[i] = 0; }
	for (size_t i = 0; i < AAD_LENGTH; ++i) { m_sdds_template.aad[0] = 0; }
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::setSddsHeaderFromSri() {
	if (not m_sdds_header_override.enabled) {
		m_sdds_template.cx = m_sri.mode;
		// This is the frequency of the digitizer clock which is twice the sample frequency if complex.
		double freq = (m_sri.mode == 1) ? (2.0 / m_sri.xdelta) : (1.0 / m_sri.xdelta);
		m_sdds_template.set_freq(freq);
	}

	if (m_first_run) {
		m_sdds_template.sos = 1;
		m_first_run = false;
	}
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::setSddsTimestamp() {
	double seconds_since_new_year = m_current_time.twsec - getStartOfYear();
	if (seconds_since_new_year < 0) {
		LOG_WARN(BulkIOToSDDSProcessor, "Cannot properly convert BulkIOTime to SDDS, the BulkIO timestamp is not from this year.");
	}
	SDDSTime sdds_time(seconds_since_new_year, m_current_time.tfsec);
	m_sdds_template.set_SDDSTime(sdds_time);
	if (not m_sdds_header_override.enabled) {
		m_sdds_template.set_ttv((m_current_time.tcstatus == BULKIO::TCS_VALID));
	}
}

template <class STREAM_TYPE>
time_t BulkIOToSDDSProcessor<STREAM_TYPE>::getStartOfYear(){
	time_t systemtime;
	tm *systemtime_struct;

	time(&systemtime);

	/* System Time in a struct of day, month, year */
	systemtime_struct = gmtime(&systemtime);

	/* Find time from EPOCH to Jan 1st of current year */
	systemtime_struct->tm_sec=0;
	systemtime_struct->tm_min=0;
	systemtime_struct->tm_hour=0;
	systemtime_struct->tm_mday=1;
	systemtime_struct->tm_mon=0;

	return (mktime(systemtime_struct) - timezone);
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::setOverrideHeaderSettings(override_sdds_header_struct sdds_header_override) {
	m_sdds_header_override = sdds_header_override;
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::setAttachSettings(sdds_attach_settings_struct attach_settings) {
	m_attach_settings = attach_settings;
}

template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::pushSri() {
	pushSri((BULKIO::dataSDDS::_ptr_type)NULL);
}
template <class STREAM_TYPE>
void BulkIOToSDDSProcessor<STREAM_TYPE>::pushSri(BULKIO::dataSDDS::_ptr_type sdds_input_port) {
	LOG_DEBUG(BulkIOToSDDSProcessor, "Pushing SRI to downstream components");
	if (not m_active_stream) {
		LOG_DEBUG(BulkIOToSDDSProcessor, "No active stream so pushSRI will not go through");
		return;
	}

	if (m_attach_settings.downstream_give_sri_priority) {
		addModifyKeyword<long>(&m_sri,"BULKIO_SRI_PRIORITY",CORBA::Long(1));
	}

	if (m_user_settings.endian_representation) {
		addModifyKeyword<long>(&m_sri,"DATA_REF_STR",CORBA::Long(DATA_REF_STR_BIG));
	} else {
		addModifyKeyword<long>(&m_sri,"DATA_REF_STR",CORBA::Long(DATA_REF_STR_LITTLE));
	}

	if (sdds_input_port == NULL) {
		m_sdds_out_port->pushSRI(m_sri, m_current_time);
	} else {
		sdds_input_port->pushSRI(m_sri, m_current_time);
	}
}

template class BulkIOToSDDSProcessor<bulkio::InShortStream>;
template class BulkIOToSDDSProcessor<bulkio::InFloatStream>;
template class BulkIOToSDDSProcessor<bulkio::InOctetStream>;
