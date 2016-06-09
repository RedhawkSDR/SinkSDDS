/*
 * BulkIOToSDDSProcessor.cpp
 *
 *  Created on: May 26, 2016
 *      Author: ylbagou
 */

#include "BulkIOToSDDSProcessor.h"

PREPARE_LOGGING(BulkIOToSDDSProcessor)

BulkIOToSDDSProcessor::BulkIOToSDDSProcessor(Resource_impl *parent, bulkio::OutSDDSPort * dataSddsOut):
m_sdds_out_port(dataSddsOut), m_shutdown(false), m_running(false), m_processorThread(NULL), m_activeStream(NOT_SET), m_parent(parent), m_first_run(true), m_seq(0), m_block_clock_drift(0.0), m_vlan(0) {

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

void BulkIOToSDDSProcessor::initializeSDDSHeader(){
	m_sdds_template.pp = 0;  //PP Parity Packet
	m_sdds_template.ssd[0] = 0;  //SSD Synchronous Serial Data
	m_sdds_template.aad[0] = 0;  //AAD Asynchronous Auxiliary Data
}

void BulkIOToSDDSProcessor::setSddsSettings(sdds_settings_struct settings) {
	//TODO: Locking?
	m_sdds_template.sf = settings.standard_format;
	m_sdds_template.of = settings.original_format;
	m_sdds_template.ss = settings.spectral_sense;
	m_user_settings = settings;
}

BulkIOToSDDSProcessor::~BulkIOToSDDSProcessor() {
	// Lets hope someone released the port lock
	if (m_processorThread) {
		join();
	}
}

void BulkIOToSDDSProcessor::run() {
	LOG_TRACE(BulkIOToSDDSProcessor, "Entering the Run Method");
	if (m_processorThread) {
		if (m_running) {
			LOG_ERROR(BulkIOToSDDSProcessor, "The BulkIO To SDDS Processor is already running, cannot start a new stream without first receiving an EOS!");
			//TODO: Exception!
			return;
		} else {
			join();
		}
	}

	if (m_activeStream == NOT_SET) {
		return;
	}

	callAttach();

	m_processorThread = new boost::thread(boost::bind(&BulkIOToSDDSProcessor::_run, boost::ref(*this)));
	LOG_TRACE(BulkIOToSDDSProcessor, "Leaving the Run Method");
}

void BulkIOToSDDSProcessor::callDettach() {
	//TODO: If the component is shutting down is this going to cause issues?
	char* id;
	switch (m_activeStream) {
		case FLOAT_STREAM:
			id = CORBA::string_dup(m_floatStream.streamID().c_str());
			break;
		case SHORT_STREAM:
			id = CORBA::string_dup(m_shortStream.streamID().c_str());
			break;
		case OCTET_STREAM:
			id = CORBA::string_dup(m_octetStream.streamID().c_str());
			break;
		default:
			// There are multiple places were detach can be called during the shutdown process so just return if we've already cleared out the stream.
			// TODO: Probably should unify where the callDetach stuff is invoked so that there isn't multiple calls to it during shutDown.
			return;
			break;
		}

		m_sdds_out_port->detach(id);
}

void BulkIOToSDDSProcessor::callAttach() {
	BULKIO::SDDSStreamDefinition sdef;
	switch (m_activeStream) {
	case FLOAT_STREAM:
		sdef.id = CORBA::string_dup(m_floatStream.streamID().c_str());
		sdef.dataFormat = (m_floatStream.sri().mode == 1) ? BULKIO::SDDS_CF : BULKIO::SDDS_SF;
		sdef.sampleRate = 1.0/m_floatStream.sri().xdelta;
		break;
	case SHORT_STREAM:
		sdef.id = CORBA::string_dup(m_shortStream.streamID().c_str());
		sdef.dataFormat = (m_shortStream.sri().mode == 1) ? BULKIO::SDDS_CI : BULKIO::SDDS_SI;
		sdef.sampleRate = 1.0/m_shortStream.sri().xdelta;
		break;
	case OCTET_STREAM:
		sdef.id = CORBA::string_dup(m_octetStream.streamID().c_str());
		sdef.dataFormat = (m_octetStream.sri().mode == 1) ? BULKIO::SDDS_CB : BULKIO::SDDS_SB;
		sdef.sampleRate = 1.0/m_octetStream.sri().xdelta;
		break;
	default:
		LOG_ERROR(BulkIOToSDDSProcessor, "Tried calling attach when no stream is currently set");
		return;
		break;
	}

	sdef.timeTagValid = m_user_settings.time_tag_valid;
	sdef.multicastAddress = CORBA::string_dup(inet_ntoa(m_connection.addr.sin_addr));
	sdef.vlan = m_vlan;
	sdef.port = ntohs(m_connection.addr.sin_port);

	m_sdds_out_port->attach(sdef, m_user_settings.user_id.c_str());
}

void BulkIOToSDDSProcessor::setSddsHeaderFromSri() {
	m_sdds_template.cx = m_sri.mode;
	// This is the frequency of the digitizer clock which is twice the sample frequency if complex.
	double freq = (m_sri.mode == 1) ? (2.0 / m_sri.xdelta) : (1.0 / m_sri.xdelta);
	m_sdds_template.set_freq(freq);
	if (m_first_run) {
		m_sdds_template.sos = 1;
	}
}

void BulkIOToSDDSProcessor::_run() {
	LOG_TRACE(BulkIOToSDDSProcessor, "Entering the _run Method");
	m_running = true;
	char *sddsDataBlock;
	int bytes_read = 0;

	while (not m_shutdown) {
		bool sriChanged = false;
		bytes_read = getDataPointer(&sddsDataBlock, sriChanged);
		LOG_TRACE(BulkIOToSDDSProcessor, "Received " << bytes_read << " bytes from bulkIO");
		if (sriChanged) {
			LOG_INFO(BulkIOToSDDSProcessor, "Stream SRI has changed, updating SDDS header.");
			m_sdds_out_port->pushSRI(m_sri, m_current_time);
			setSddsHeaderFromSri();
		}

		if (not bytes_read) {
			//TODO: LOG and send an EOS
			callDettach();
			m_activeStream = NOT_SET;
			m_shutdown = true;
			continue;
		}

		if (sendPacket(sddsDataBlock, bytes_read) < 0) {
			LOG_ERROR(BulkIOToSDDSProcessor, "Failed to push packet over socket, stream will be closed.");
			callDettach();
			m_activeStream = NOT_SET;
			m_shutdown = true;
			continue;
		}

		m_first_run = false; // TODO: Can we not set this every single time we run this loop? Seems silly.
	}

	m_first_run = true;
	m_shutdown = false;
	m_running = false;
	LOG_TRACE(BulkIOToSDDSProcessor, "Exiting the _run Method");
}


int BulkIOToSDDSProcessor::sendPacket(char* dataBlock, int num_bytes) {
/**
 * Four different situations covered here.
 * 1. We've read exactly 1024 bytes. Occurs when the mode is real, we get exactly one packets worth and this method runs only once.
 * 2. We've read exactly 2*1024 bytes. Occurs when the mode is complex, this method runs twice recursively. Decreasing the num_bytes
 * 3. We've read less less than 1024 bytes, occurs when the stream is changing.
 * 4. We've read more than 1024, but less than 2*1024.
 */

	LOG_TRACE(BulkIOToSDDSProcessor, "sendPacket called, told to send " << num_bytes <<  "bytes");
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

	LOG_TRACE(BulkIOToSDDSProcessor, "Pushed " << numSent << " bytes out of socket.")

	// Its possible we read more than a single packet if the mode changed on us (known bug)
	return sendPacket(dataBlock + SDDS_DATA_SIZE, num_bytes - m_msg_iov[1].iov_len);
}

void BulkIOToSDDSProcessor::setSddsTimestamp() {
	SDDSTime sdds_time(m_current_time.twsec - getStartOfYear(), m_current_time.tfsec);
	m_sdds_template.set_SDDSTime(sdds_time);
	m_sdds_template.set_ttv((m_current_time.tcstatus == BULKIO::TCS_VALID));
	m_sdds_template.set_dfdt(m_block_clock_drift);
}

time_t BulkIOToSDDSProcessor::getStartOfYear(){
	time_t systemtime;
	tm *systemtime_struct;

	time(&systemtime);

	/* System Time in a struct of day, month, year */
	systemtime_struct = localtime(&systemtime);

	/* Find time from EPOCH to Jan 1st of current year */
	systemtime_struct->tm_sec=0;
	systemtime_struct->tm_min=0;
	systemtime_struct->tm_hour=0;
	systemtime_struct->tm_mday=1;
	systemtime_struct->tm_mon=0;
	return mktime(systemtime_struct);
}

void BulkIOToSDDSProcessor::shutdown() {
	callDettach();
	m_shutdown = true;
}

void BulkIOToSDDSProcessor::join() {
	if (m_processorThread) {
		m_processorThread->join();
		delete(m_processorThread);
		m_processorThread = NULL;
	} else {
		LOG_DEBUG(BulkIOToSDDSProcessor, "Join called but the thread object is null.");
	}
}

void BulkIOToSDDSProcessor::setConnection(connection_t connection, uint16_t vlan) {
	m_connection = connection;
	m_vlan = vlan;
	// Not sure why but the msg_name and msg_namelen must be set. Perhaps it's a UDP thing. I thought they were optional
	m_pkt_template.msg_name = &m_connection.addr;
	m_pkt_template.msg_namelen = sizeof(m_connection.addr);
}

// TODO: Reread the spec, this doesnt give it in the right units
double BulkIOToSDDSProcessor::getClockDrift(std::list<bulkio::SampleTimestamp> ts, size_t numSamples) {
	if (ts.size() == 1) {
		return 0.0;
	}

	// The last time stamp does not indicate the time stamp of the last sample, it maps to the offset.
	// If we collect 512 Samples, and the last time stamp is for 511th sample (indexed by 0) then we
	// actually do have the last timestamp, otherwise we need to account for and estimate the last time stamp.
	int num_samples_missing_timestamps = numSamples - (ts.back().offset + 1);
	BULKIO::PrecisionUTCTime last_sample_ts = ts.back().time + num_samples_missing_timestamps * m_sri.xdelta;

	double pkt_delta = last_sample_ts - ts.front().time;
	double expected_pkt_delta = ts.front().time + numSamples*m_sri.xdelta - ts.front().time;

	return expected_pkt_delta - pkt_delta;
}







//TODO: SOOOOO MUCH COPY PASTE!
// If the mode starts as complex we will see the mode reflected as complex correctly in the block SRI
// If the mode starts as real we will see the mode reflected as real correctly in the block SRI
// If the mode changes from real to complex, or from complex to real, we will NOT see the correct mode prior to the read call.
// This causes problems in rare cases were the mode changes in a stream, going from real -> complex we will end up with twice the number of byes we would expect
// Going from complex to real we end up with half the number of bytes we would expect.
// A stream changing from real to complex or complex to real should almost never happen though.
size_t BulkIOToSDDSProcessor::getDataPointer(char **dataPointer, bool &sriChanged) {
	size_t bytes_read = 0;
	size_t complex_scale = 0;

	switch (m_activeStream) {
	case FLOAT_STREAM:
		complex_scale = (m_floatStream.sri().mode == 0 ? 1 : 2);
		m_floatBlock = m_floatStream.read(SDDS_DATA_SIZE / sizeof(float) / complex_scale);
		if (!!m_floatBlock) {  //TODO: Document bang bang
			m_current_time = m_floatBlock.getTimestamps().front().time;
			m_block_clock_drift = getClockDrift(m_floatBlock.getTimestamps(), SDDS_DATA_SIZE / sizeof(float) / complex_scale);

			bytes_read = m_floatBlock.size() * sizeof(float);
			*dataPointer = reinterpret_cast<char*>(m_floatBlock.data());
			if (m_user_settings.endian_representation) { // 1 for Big 0 for Little, we assume we're running on a little endian so we byteswap
				uint32_t *buf = reinterpret_cast<uint32_t*>(*dataPointer);
				for (size_t i = 0; i < m_floatBlock.size(); ++i) {
					buf[i] = __builtin_bswap32(buf[i]);
				}
			}
			if (m_first_run || m_floatBlock.sriChanged()) {
				m_sri = m_floatBlock.sri();
				sriChanged = true;
				if (m_floatBlock.sriChangeFlags() & bulkio::sri::MODE) {
					LOG_ERROR(BulkIOToSDDSProcessor, "KNOWN ISSUE!! Mode was changed in the middle of a stream. "
							"Going from Real->Complex this will cause 1 SDDS packets worth of data to have an incorrect time stamp. "
							"Going from Complex->Real will cause a single packet to be erroneously padded with zeros.");
				}
			}
		}
		break;
	case SHORT_STREAM:
		complex_scale = (m_shortStream.sri().mode == 0 ? 1 : 2);
		m_shortBlock = m_shortStream.read(SDDS_DATA_SIZE / sizeof(short) / complex_scale);
		if (!!m_shortBlock) {
			m_current_time = m_shortBlock.getTimestamps().front().time;
			m_block_clock_drift = getClockDrift(m_shortBlock.getTimestamps(), SDDS_DATA_SIZE / sizeof(short) / complex_scale);
			bytes_read = m_shortBlock.size() * sizeof(short);
			*dataPointer = reinterpret_cast<char*>(m_shortBlock.data());
			if (m_user_settings.endian_representation) {
				swab(*dataPointer, *dataPointer, bytes_read);
			}
			if (m_first_run || m_shortBlock.sriChanged()) {
				m_sri = m_shortBlock.sri();
				sriChanged = true;
				if (m_shortBlock.sriChangeFlags() & bulkio::sri::MODE) {
					LOG_ERROR(BulkIOToSDDSProcessor, "KNOWN ISSUE!! Mode was changed in the middle of a stream. "
							"Going from Real->Complex this will cause 1 SDDS packets worth of data to have an incorrect time stamp. "
							"Going from Complex->Real will cause a single packet to be erroneously padded with zeros.");
				}
			}
		}
		break;
	case OCTET_STREAM:
		complex_scale = (m_octetStream.sri().mode == 0 ? 1 : 2);
		m_octetBlock = m_octetStream.read(SDDS_DATA_SIZE / sizeof(char) / complex_scale);
		if (!!m_octetBlock) {
			m_current_time = m_octetBlock.getTimestamps().front().time;
			m_block_clock_drift = getClockDrift(m_octetBlock.getTimestamps(), SDDS_DATA_SIZE / sizeof(char) / complex_scale);
			bytes_read = m_octetBlock.size() * sizeof(char);
			*dataPointer = reinterpret_cast<char*>(m_octetBlock.data());
			if (m_first_run || m_octetBlock.sriChanged()) {
				m_sri = m_octetBlock.sri();
				sriChanged = true;
				if (m_octetBlock.sriChangeFlags() & bulkio::sri::MODE) {
					LOG_ERROR(BulkIOToSDDSProcessor, "KNOWN ISSUE!! Mode was changed in the middle of a stream. "
							"Going from Real->Complex this will cause 1 SDDS packets worth of data to have an incorrect time stamp. "
							"Going from Complex->Real will cause a single packet to be erroneously padded with zeros.");
				}
			}
		}
		break;
	case NOT_SET:
		LOG_ERROR(BulkIOToSDDSProcessor, "Processor thread running but the active stream is not set");
		break;
	default:
		LOG_ERROR(BulkIOToSDDSProcessor, "Processor thread running but the active stream is not set");
		break;
	};

	return bytes_read;
}



//********************************************************************************************************
// TODO: Feels so gross having essentially the same 2 functions 3 times each. Anyway to fix it?
//********************************************************************************************************

// ***************************** Set Streams ******************************
void BulkIOToSDDSProcessor::setFloatStream(bulkio::InFloatStream stream) {
	boost::unique_lock<boost::mutex> lock(m_streamLock);
	LOG_INFO(BulkIOToSDDSProcessor, "Received new stream: " << stream.streamID());
	if (m_activeStream == NOT_SET) {
		m_activeStream = FLOAT_STREAM;
		m_floatStream = stream;
		m_sdds_template.bps = SDDS_FLOAT_BPS;
		if (m_parent->started()) { run(); }
	} else {
		LOG_ERROR(BulkIOToSDDSProcessor, "There is already an active stream named: " << m_floatStream.streamID() << ", cannot set new stream: " << stream.streamID());
	}
}

void BulkIOToSDDSProcessor::setShortStream(bulkio::InShortStream stream) {
	LOG_INFO(BulkIOToSDDSProcessor, "Received new stream: " << stream.streamID());
	boost::unique_lock<boost::mutex> lock(m_streamLock);
	if (m_activeStream == NOT_SET) {
		m_activeStream = SHORT_STREAM;
		m_shortStream = stream;
		m_sdds_template.bps = SDDS_SHORT_BPS;
		if (m_parent->started()) { run(); }
	} else {
		LOG_ERROR(BulkIOToSDDSProcessor, "There is already an active stream named: " << m_shortStream.streamID() << ", cannot set new stream: " << stream.streamID());
	}
}

void BulkIOToSDDSProcessor::setOctetStream(bulkio::InOctetStream stream) {
	LOG_INFO(BulkIOToSDDSProcessor, "Received new stream: " << stream.streamID());
	boost::unique_lock<boost::mutex> lock(m_streamLock);
	if (m_activeStream == NOT_SET) {
		m_activeStream = OCTET_STREAM;
		m_octetStream = stream;
		m_sdds_template.bps = SDDS_OCTET_BPS;
		if (m_parent->started()) { run(); }
	} else {
		LOG_ERROR(BulkIOToSDDSProcessor, "There is already an active stream named: " << m_octetStream.streamID() << ", cannot set new stream: " << stream.streamID());
	}
}




// ***************************** Remove Streams ******************************

void BulkIOToSDDSProcessor::removeFloatStream(bulkio::InFloatStream stream) {
	LOG_INFO(BulkIOToSDDSProcessor, "Removing stream: " << stream.streamID());
	boost::unique_lock<boost::mutex> lock(m_streamLock);
	if (m_activeStream == FLOAT_STREAM && m_floatStream.streamID() == stream.streamID()) {
		m_activeStream = NOT_SET;
		if (m_running) {
			shutdown();
			join();
		}
	} else {
		LOG_WARN(BulkIOToSDDSProcessor, "Was told to remove stream that was not already set.");
	}
}


void BulkIOToSDDSProcessor::removeShortStream(bulkio::InShortStream stream) {
	LOG_INFO(BulkIOToSDDSProcessor, "Removing stream: " << stream.streamID());
	boost::unique_lock<boost::mutex> lock(m_streamLock);
	if (m_activeStream == SHORT_STREAM && m_shortStream.streamID() == stream.streamID()) {
		m_activeStream = NOT_SET;
		if (m_running) {
			shutdown();
			join();
		}
	} else {
		LOG_WARN(BulkIOToSDDSProcessor, "Was told to remove stream that was not already set.");
	}
}

void BulkIOToSDDSProcessor::removeOctetStream(bulkio::InOctetStream stream) {
	LOG_INFO(BulkIOToSDDSProcessor, "Removing stream: " << stream.streamID());
	boost::unique_lock<boost::mutex> lock(m_streamLock);
	if (m_activeStream == OCTET_STREAM && m_octetStream.streamID() == stream.streamID()) {
		m_activeStream = NOT_SET;
		if (m_running) {
			shutdown();
			join();
		}
	} else {
		LOG_WARN(BulkIOToSDDSProcessor, "Was told to remove stream that was not already set.");
	}
}

