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
#include "StreamsDoneCallBackInterface.h"

#define SDDS_DATA_SIZE 1024
#define SDDS_HEADER_SIZE 56
#define SSD_LENGTH 2
#define AAD_LENGTH 20
#define DATA_REF_STR_LITTLE 43981
#define DATA_REF_STR_BIG 52651

template <class STREAM_TYPE>
class BulkIOToSDDSProcessor {
	ENABLE_LOGGING

	typedef typename STREAM_TYPE::DataBlockType BLOCK_TYPE;
	typedef typename STREAM_TYPE::NativeType NATIVE_TYPE;

public:
	BulkIOToSDDSProcessor(bulkio::OutSDDSPort * dataSddsOut, StreamsDoneCallBackInterface * parent);
	~BulkIOToSDDSProcessor();
	bool isActive();
	void join();
	void setSddsSettings(sdds_settings_struct settings);
	void run();
	void callDetach();
	void callAttach();
	void callAttach(BULKIO::dataSDDS::_ptr_type sdds_input_port);
	void pushSri();
	void pushSri(BULKIO::dataSDDS::_ptr_type sdds_input_port);
	void setConnection(connection_t connection, uint16_t vlan);
	void setOverrideHeaderSettings(override_sdds_header_struct sdds_header_override);
	void setAttachSettings(sdds_attach_settings_struct attach_settings);
	void shutdown();
	void setStream(STREAM_TYPE stream);
	void removeStream(STREAM_TYPE stream);
	std::string getStreamId();

private:
	void _run();
	size_t getDataPointer(char **dataPointer, bool &sriChanged);
	int sendPacket(char* dataBlock, int num_bytes);
	void initializeSDDSHeader();
	void setSddsHeaderFromSri();
	void setSddsTimestamp();
	time_t getStartOfYear();
	void overrideSddsHeader();

	bulkio::OutSDDSPort *m_sdds_out_port;
	bool m_first_run;
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
	override_sdds_header_struct m_sdds_header_override;
	sdds_attach_settings_struct m_attach_settings;
	StreamsDoneCallBackInterface * m_parent;

	template <typename CORBAXX>
		bool addModifyKeyword(BULKIO::StreamSRI *sri, CORBA::String_member id, CORBAXX myValue, bool addOnly = false) {
			CORBA::Any value;
			value <<= (CORBAXX) myValue;
			unsigned long keySize = sri->keywords.length();
			if (!addOnly) {
				for (unsigned int i = 0; i < keySize; i++) {
					if (!strcmp(sri->keywords[i].id, id)) {
						sri->keywords[i].value = value;
						return true;
					}
				}
			}
			sri->keywords.length(keySize + 1);
			if (sri->keywords.length() != keySize + 1)
				return false;
			sri->keywords[keySize].id = CORBA::string_dup(id);
			sri->keywords[keySize].value = value;
			return true;
		}
};

#endif /* BULKIOTOSDDSPROCESSOR_H_ */
