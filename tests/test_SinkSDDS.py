#!/usr/bin/env python

import ossie.utils.testing
from ossie.utils import sb
import unicast
import time
import socket
import struct
import Sdds
import binascii

DEBUG_LEVEL = 3
UNICAST_PORT = 1234
UNICAST_IP = '127.0.0.1'

# TODO: I changed the inheritance from RHTest to ScaComponentTestCase, put back after figuring out how to deal with
# the new type
class ComponentTests(ossie.utils.testing.ScaComponentTestCase):
    # Path to the SPD file, relative to this file. This must be set in order to
    # launch the component.
    SPD_FILE = '../SinkSDDS.spd.xml'

# TODO: Standard Format check
# TODO: Original Format check
# TODO: Spectral Sense check
# TODO: Attach tests
# TODO: df/dt drift test
# TODO: Multiple streams added
# TODO: Add/Remove stream.
# TODO: Endianess
# TODO: Time code valid checks


    # This unit test takes FOREVER (going through all the packets) how can we speed this thing up?
#     def testSoS(self):
#         self.octetConnect()
#         sb.start()
#         fakeData = 1024*[10];
#         seq = 0
#         while seq < 65536:
#             self.sendPacket(fakeData, 1.0, False)
#             recv = self.getPacket(10000)
#             h = self.getHeader(recv)
#             self.assertTrue(h.SoS == 1, "SoS bit should be set")
#             self.assertEqual(h.fsn, seq, "Received sequence number did not match expected")
#             seq = seq + 1
#             if (seq != 0 and seq % 32 == 31):
#                  seq = seq + 1
#             print h.fsn
#                  
#         time.sleep(1) # Let the packets finish up.
#         self.sendPacket(fakeData, 1.0, False)
#         recv = self.getPacket(10000)
#         h = self.getHeader(recv)
#         
#         self.assertTrue(h.SoS == 0, "SoS bit should not be set")
            

    def testTimeDrift(self):
        self.octetConnect()
        sb.start()
        fakeData = 100*[10];
        for x in range(100):
            self.sendPacket(fakeData, 1.0, True, eos=False)
        
        time.sleep(1)
            
        
        
    def testCx(self):
        self.octetConnect()
        sb.start()
        fakeData = 1024*[10];
        self.sendPacket(fakeData, 1.0, False, eos=False)
        recv = self.getPacket(10000)
        cx = self.getCx(recv)
        self.assertEqual(cx, 0, "Expected complex bit to be zero")
        self.sendPacket(fakeData, 1.0, True, eos=True)
        time.sleep(0.5)
        recv = self.getPacket(10000)
        cx = self.getCx(recv)
        self.assertEqual(cx, 1, "Expected complex bit to be one")
        
    def testFSN(self):
        expected_fsn = 0
        self.octetConnect()
        sb.start()
        fakeData = 1024*[10];
        for x in range(100):
          self.sendPacket(fakeData, 1.0, False)
          recv = self.getPacket(10000)
          sdds_header = self.getHeader(recv)
          self.assertEqual(sdds_header.fsn, expected_fsn, "Frame sequence numbers do not match")
          expected_fsn += 1
          if expected_fsn != 0 and expected_fsn % 32 == 31:
            expected_fsn += 1
        
    def testXDeltaChange(self):
        self.octetConnect();
        sb.start()
        
        #  We expect to get 1 packet of 1024 9's then a packet of a single 9, and 1023 zeros, then a packet of 1024 9's
        fakeData1 = 1025*[9]
        self.sendPacket(fakeData1, 1000.0, False)
        fakeData2 = 1024*[9]
        self.sendPacket(fakeData2, 2000, False)
        
        expected1 = 1024*[9]
        expected2 = 1*[9] + 1023*[0]
        expected3 = 1024*[9]
        
        recv = self.getPacket(10000)
        self.assertEqual(expected1, list(struct.unpack('1024B', recv[-1024:])))
        recv = self.getPacket(10000)
        self.assertEqual(expected2, list(struct.unpack('1024B', recv[-1024:])))
        recv = self.getPacket(10000)
        self.assertEqual(expected3, list(struct.unpack('1024B', recv[-1024:])))

    def testRealOctet(self):
        self.dataOctetInTest(False)
    def testComplexOctet(self):
        self.dataOctetInTest(True)
        
    def testRealShort(self):
        self.dataShortInTest(False)
    def testComplexShort(self):
        self.dataShortInTest(True)
        
    def testRealFloat(self):
        self.dataFloatInTest(False)
    def testComplexFloat(self):
        self.dataFloatInTest(True)
        
    def dataOctetInTest(self, is_complex):
        self.octetConnect()
        sb.start()
        
        # Create data
        fakeData = [x % 256 for x in range(1024)]
        recv = self.sendPacketGetPacket(fakeData, is_complex=is_complex, eos=True)
        
        # Validate correct amount of data was received
        self.assertEqual(len(recv), 1080)
        
        # Validate data is correct
        self.assertEqual(fakeData, list(struct.unpack('1024B', recv[-1024:])))

    def dataShortInTest(self, is_complex):
        self.shortConnect()
        sb.start()
        
        # Create data
        fakeData = [x % 256 for x in range(1024/2)]
        recv = self.sendPacketGetPacket(fakeData, is_complex=is_complex, eos=True)
        
        # Validate correct amount of data was received
        self.assertEqual(len(recv), 1080)
        
        # Validate data is correct
        self.assertEqual(fakeData, list(struct.unpack('!512H', recv[-1024:])))

    def dataFloatInTest(self, is_complex):
        self.floatConnect()
        sb.start()
        
        # Create data
        fakeData = [x % 256 for x in range(1024/4)]
        recv = self.sendPacketGetPacket(fakeData, is_complex=is_complex, eos=True)
        
        # Validate correct amount of data was received
        self.assertEqual(len(recv), 1080)
        
        # Validate data is correct
        self.assertEqual(fakeData, list(struct.unpack('!256f', recv[-1024:])))
        
    def sendPacketGetPacket(self, data_to_send, sample_rate=1.0, is_complex=False, socket_read_size=1080, eos=False):
        self.sendPacket(data_to_send, sample_rate, is_complex, eos)
        # Wait for data
        time.sleep(0.1)
        return self.getPacket(socket_read_size)
        
    def sendPacket(self, data_to_send, sample_rate=1.0, is_complex=False, eos=False):
        self.source.push(data_to_send, eos, self.id(), sample_rate, is_complex)
        
    def getPacket(self, socket_read_size=1080):
        # Get data
        received_data = []
        try:
             received_data = self.uclient.receive(socket_read_size)
        except socket.error as e:
            print "Socket read error: ", e
        
        return received_data
    
    def floatConnect(self):
        self.source.connect(self.comp, usesPortName='floatOut')
    def shortConnect(self):
        self.source.connect(self.comp, usesPortName='shortOut')
    def octetConnect(self):
        self.source.connect(self.comp, usesPortName='octetOut')

    def tearDown(self):
        # Clean up all sandbox artifacts created during test
        sb.release()
        del(self.uclient)

    def testBasicBehavior(self):
        #######################################################################
        # Make sure start and stop can be called without throwing exceptions
        self.comp.start()
        self.comp.stop()

    def setUp(self):
        print "\nRunning test:", self.id()
        ossie.utils.testing.ScaComponentTestCase.setUp(self)

        #Launch the component with the default execparams.
        execparams = self.getPropertySet(kinds = ("execparam",), modes = ("readwrite", "writeonly"), includeNil = False)
        execparams = dict([(x.id, any.from_any(x.value)) for x in execparams])
        execparams["DEBUG_LEVEL"] = DEBUG_LEVEL 
        self.launch(execparams)

        #Simulate regular component startup.
        #Verify that initialize nor configure throw errors.
        self.comp.initialize()
        configureProps = self.getPropertySet(kinds = ("configure",), modes = ("readwrite", "writeonly"), includeNil = False)
        self.comp.configure(configureProps)

        #Configure unicast properties
        self.comp.network_settings.interface = 'lo'
        self.comp.network_settings.ip_address = UNICAST_IP
        self.comp.network_settings.port = UNICAST_PORT
        self.comp.network_settings.vlan = 0
        self.uclient = unicast.unicast_client(UNICAST_IP, UNICAST_PORT)
        self.source = sb.DataSource();

    def getHeader(self, p):
        SF_MASK = (1   << 7 + 8)
        SOS_MASK = (1  << 6 + 8)
        PP_MASK = (1   << 5 + 8)
        OF_MASK = (1   << 4 + 8)
        SS_MASK = (1   << 3 + 8)
        DM_MASK = (7   << 0 + 8)
        
        CX_MASK = (1   << 7)
        BPS_MASK = (31 << 0)
        
        MSV_MASK = 0x0080
        TTV_MASK = 0x0040
        SSV_MASK = 0x0020
        
        raw_header = struct.unpack('>HHHHQLLQ24B', p[:56])
        SF = (raw_header[0] & SF_MASK) >> (7)
        SoS = (raw_header[0] & SOS_MASK) >> (6+8)
        PP = (raw_header[0] & PP_MASK) >> (5 + 8)
        OF = (raw_header[0] & OF_MASK) >> (4 + 8)
        SS = (raw_header[0] & SS_MASK) >> (3 + 8)
        DM = (raw_header[0] & DM_MASK) >> (0 + 8)
        DM = [DM & 1<<2, DM & 1<<1, DM & 1<<0]
        CX = raw_header[0] & CX_MASK
        BPS = raw_header[0] & BPS_MASK
        BPS = [BPS & 1<<4, BPS & 1<<3, BPS & 1<<2, BPS & 1<<1, BPS & 1<<0]
        
        FSN = raw_header[1]
        
        # TODO: Fix these by shifting them down
        MSV = raw_header[2] & MSV_MASK
        TTV = raw_header[2] & TTV_MASK
        SSV = raw_header[2] & SSV_MASK
        
        MSD = raw_header[3]
        TT = raw_header[4]
        TTE = raw_header[5]
        DFDT = raw_header[6]
        FREQ = raw_header[7]
         
        return Sdds.SddsHeader(FSN, SF, SoS, PP, OF, SS, DM, BPS, MSV, TTV, SSV, MSD, TT, TTE, DFDT, FREQ)

    def getCx(self, p):
        CX_MASK = (1 << (0 + 7))
        raw_header = struct.unpack('>HHHHQLLQ24B', p[:56])
        return (raw_header[0] & CX_MASK) >> 7
        

if __name__ == "__main__":
    ossie.utils.testing.main("../SinkSDDS.spd.xml") # By default tests all implementations
