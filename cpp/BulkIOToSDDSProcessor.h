#ifndef BULKIOTOSDDSPROCESSOR_H_
#define BULKIOTOSDDSPROCESSOR_H_

#include <bulkio/bulkio.h>
#include "struct_props.h"
#include <boost/thread.hpp>
#include <ossie/debug.h>
#include <Resource_impl.h>
#include "sddspacket.h"
#include "socketUtils/multicast.h"
#include "socketUtils/unicast.h"
#include "struct_props.h"

#define SDDS_DATA_SIZE 1024
#define SDDS_HEADER_SIZE 56

template <class STREAM_TYPE>
class BulkIOToSDDSProcessor {
	typedef typename STREAM_TYPE::DataBlockType BLOCK_TYPE;
	typedef typename STREAM_TYPE::NativeType NATIVE_TYPE;

public:
	BulkIOToSDDSProcessor(Resource_impl *parent, bulkio::OutSDDSPort * dataSddsOut):
		m_parent(parent), m_sdds_out_port(dataSddsOut), m_first_run(true), m_block_clock_drift(0.0), m_processorThread(NULL), m_shutdown(false), m_running(false), m_active_stream(false),  m_vlan(0), m_seq(0) {

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
	virtual ~BulkIOToSDDSProcessor() {
		// Lets hope someone released the port lock
		if (m_processorThread) {
			join();
		}
	}

	bool isActive() {
		return m_active_stream;
	}

	void join() {
		if (m_processorThread) {
			m_processorThread->join();
			delete(m_processorThread);
			m_processorThread = NULL;
		} else {
			RH_NL_DEBUG("BulkIOToSDDSProcessor", "Join called but the thread object is null.");
		}
	}

	void setSddsSettings(sdds_settings_struct settings) {
		//TODO: Locking?
		m_sdds_template.sf = settings.standard_format;
		m_sdds_template.of = settings.original_format;
		m_sdds_template.ss = settings.spectral_sense;
		m_user_settings = settings;
	}

	void run() {
		RH_NL_TRACE("BulkIOToSDDSProcessor", "Entering the Run Method");
		if (m_processorThread) {
			if (m_running) {
				RH_NL_ERROR("BulkIOToSDDSProcessor", "The BulkIO To SDDS Processor is already running, cannot start a new stream without first receiving an EOS!");
				//TODO: Exception!
				return;
			} else {
				join();
			}
		}

		if (not m_active_stream) {
			return;
		}

		callAttach();

		m_processorThread = new boost::thread(boost::bind(&BulkIOToSDDSProcessor<STREAM_TYPE>::_run, boost::ref(*this)));
		RH_NL_TRACE("BulkIOToSDDSProcessor", "Leaving the Run Method");
	}

	void callDettach() {
		//TODO: If the component is shutting down is this going to cause issues?
		// There are multiple places were detach can be called during the shutdown process so just return if we've already cleared out the stream.
		// TODO: Probably should unify where the callDetach stuff is invoked so that there isn't multiple calls to it during shutDown.
		if (m_active_stream) {
			m_sdds_out_port->detach(CORBA::string_dup(m_stream.streamID().c_str()));
		}
	}


	void callAttach() {
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
			RH_NL_ERROR("BulkIOToSDDSProcessor", "Native type size is not what we would expect.");
			break;

		}

		sdef.sampleRate = 1.0/m_stream.sri().xdelta;
		sdef.timeTagValid = m_user_settings.time_tag_valid;
		sdef.multicastAddress = CORBA::string_dup(inet_ntoa(m_connection.addr.sin_addr));
		sdef.vlan = m_vlan;
		sdef.port = ntohs(m_connection.addr.sin_port);

		m_sdds_out_port->attach(sdef, m_user_settings.user_id.c_str());
	}

	void setConnection(connection_t connection, uint16_t vlan) {
		m_connection = connection;
		m_vlan = vlan;
		// Not sure why but the msg_name and msg_namelen must be set. Perhaps it's a UDP thing. I thought they were optional
		m_pkt_template.msg_name = &m_connection.addr;
		m_pkt_template.msg_namelen = sizeof(m_connection.addr);
	}

	void shutdown() {
		callDettach();
		m_shutdown = true;
	}

	void setStream(STREAM_TYPE stream) {
		RH_NL_INFO("BulkIOToSDDSProcessor", "Received new stream: " << stream.streamID());

		if (m_active_stream) {
			RH_NL_ERROR("BulkIOToSDDSProcessor", "There is already an active stream named: " << m_stream.streamID() << ", cannot set new stream: " << stream.streamID());
			return;
		}

		m_stream = stream;
		m_active_stream = true;
		//TODO: Add cast to get rid of the warning
		m_sdds_template.bps = (sizeof(NATIVE_TYPE) == sizeof(float)) ? (8*sizeof(NATIVE_TYPE)) : 31;
		if (m_parent->started()) { run(); }
	}

	void removeStream(STREAM_TYPE stream) {
		RH_NL_INFO("BulkIOToSDDSProcessor", "Removing stream: " << stream.streamID());
		if (m_active_stream && m_stream.streamID() == stream.streamID()) {
			if (m_running) {
				shutdown();
				join();
			}

			m_active_stream = false;
		} else {
			RH_NL_WARN("BulkIOToSDDSProcessor", "Was told to remove stream that was not already set.");
		}
	}


private:

	void _run() {
		RH_NL_TRACE("BulkIOToSDDSProcessor", "Entering the _run Method");
		m_running = true;
		char *sddsDataBlock;
		int bytes_read = 0;

		while (not m_shutdown) {
			bool sriChanged = false;
			bytes_read = getDataPointer(&sddsDataBlock, sriChanged);
			RH_NL_TRACE("BulkIOToSDDSProcessor", "Received " << bytes_read << " bytes from bulkIO");
			if (sriChanged) {
				RH_NL_INFO("BulkIOToSDDSProcessor", "Stream SRI has changed, updating SDDS header.");
				m_sdds_out_port->pushSRI(m_sri, m_current_time);
				setSddsHeaderFromSri();
			}

			if (not bytes_read) {
				//TODO: LOG and send an EOS
				callDettach();
				m_shutdown = true;
				continue;
			}

			if (sendPacket(sddsDataBlock, bytes_read) < 0) {
				RH_NL_ERROR("BulkIOToSDDSProcessor", "Failed to push packet over socket, stream will be closed.");
				callDettach();
				m_shutdown = true;
				continue;
			}

			m_first_run = false; // TODO: Can we not set this every single time we run this loop? Seems silly.
		}

		m_first_run = true;
		m_shutdown = false;
		m_running = false;
		RH_NL_TRACE("BulkIOToSDDSProcessor", "Exiting the _run Method");
	}

	size_t getDataPointer(char **dataPointer, bool &sriChanged) {
		RH_NL_TRACE("BulkIOToSDDSProcessor", "Entering getDataPointer Method");

		size_t bytes_read = 0;
		size_t complex_scale = (m_stream.sri().mode == 0 ? 1 : 2);

		m_block = m_stream.read(SDDS_DATA_SIZE / sizeof(NATIVE_TYPE) / complex_scale);


		if (!!m_block) {  //TODO: Document bang bang
			m_current_time = m_block.getTimestamps().front().time;
			m_block_clock_drift = getClockDrift(m_block.getTimestamps(), SDDS_DATA_SIZE / sizeof(NATIVE_TYPE) / complex_scale);

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
				// TODO: Need method for endianness that will be switch statement on sizeof naitive type
			}
			if (m_first_run || m_block.sriChanged()) {
				m_sri = m_block.sri();
				sriChanged = true;
				if (m_block.sriChangeFlags() & bulkio::sri::MODE) {
					RH_NL_ERROR("BulkIOToSDDSProcessor", "KNOWN ISSUE!! Mode was changed in the middle of a stream. "
							"Going from Real->Complex this will cause 1 SDDS packets worth of data to have an incorrect time stamp. "
							"Going from Complex->Real will cause a single packet to be erroneously padded with zeros.");
				}
			}
		}

		RH_NL_TRACE("BulkIOToSDDSProcessor", "Leaving getDataPointer Method");
		return bytes_read;
	}

	int sendPacket(char* dataBlock, int num_bytes) {
	/**
	 * Four different situations covered here.
	 * 1. We've read exactly 1024 bytes. Occurs when the mode is real, we get exactly one packets worth and this method runs only once.
	 * 2. We've read exactly 2*1024 bytes. Occurs when the mode is complex, this method runs twice recursively. Decreasing the num_bytes
	 * 3. We've read less less than 1024 bytes, occurs when the stream is changing.
	 * 4. We've read more than 1024, but less than 2*1024.
	 */

		RH_NL_TRACE("BulkIOToSDDSProcessor", "sendPacket called, told to send " << num_bytes <<  "bytes");
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

		RH_NL_TRACE("BulkIOToSDDSProcessor", "Pushed " << numSent << " bytes out of socket.")

		// Its possible we read more than a single packet if the mode changed on us (known bug)
		return sendPacket(dataBlock + SDDS_DATA_SIZE, num_bytes - m_msg_iov[1].iov_len);
	}

	void initializeSDDSHeader(){
		m_sdds_template.pp = 0;  //PP Parity Packet
		m_sdds_template.ssd[0] = 0;  //SSD Synchronous Serial Data
		m_sdds_template.aad[0] = 0;  //AAD Asynchronous Auxiliary Data
	}

	void setSddsHeaderFromSri() {
		m_sdds_template.cx = m_sri.mode;
		// This is the frequency of the digitizer clock which is twice the sample frequency if complex.
		double freq = (m_sri.mode == 1) ? (2.0 / m_sri.xdelta) : (1.0 / m_sri.xdelta);
		m_sdds_template.set_freq(freq);
		if (m_first_run) {
			m_sdds_template.sos = 1;
		}
	}

	void setSddsTimestamp() {
		SDDSTime sdds_time(m_current_time.twsec - getStartOfYear(), m_current_time.tfsec);
		m_sdds_template.set_SDDSTime(sdds_time);
		m_sdds_template.set_ttv((m_current_time.tcstatus == BULKIO::TCS_VALID));
		m_sdds_template.set_dfdt(m_block_clock_drift);
	}

	time_t getStartOfYear(){
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

	// TODO: Reread the spec, this doesnt give it in the right units
	double getClockDrift(std::list<bulkio::SampleTimestamp> ts, size_t numSamples) {
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

	Resource_impl *m_parent;
	bulkio::OutSDDSPort *m_sdds_out_port;
	bool m_first_run;
	double m_block_clock_drift;
	boost::thread *m_processorThread;
	bool m_shutdown, m_running, m_active_stream;
	uint16_t m_vlan;

	STREAM_TYPE m_stream;
	BLOCK_TYPE m_block;
	BULKIO::PrecisionUTCTime m_current_time;
	sdds_settings_struct m_user_settings;
	BULKIO::StreamSRI m_sri;
	SDDSpacket m_sdds_template;
	char m_zero_pad_buffer[SDDS_DATA_SIZE]; // This buffer is only used if we do a non full read off of the stream API.
	iovec m_msg_iov[3];
	msghdr m_pkt_template;
	connection_t m_connection;
	uint16_t m_seq;

};

#endif /* BULKIOTOSDDSPROCESSOR_H_ */
