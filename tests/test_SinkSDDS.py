#!/usr/bin/env python

import ossie.utils.testing
from ossie.utils import sb
import unicast
import time
import socket
import struct
import Sdds
import binascii
import sys
import random

from ossie.utils.bulkio.bulkio_data_helpers import SDDSSink

DEBUG_LEVEL = 0
UNICAST_PORT = 1234
UNICAST_IP = '127.0.0.1'

FLOAT_BPS = [16, 8, 4, 2, 1]
SHORT_BPS = [16,  0, 0, 0, 0]
OCTET_BPS = [0,  8, 0, 0, 0]

# Working around existing 2.0.1 framework sb bug where the SDDSink does not hold on to the sri
class SDDSSink2(SDDSSink):
    
    def pushSRI(self, H, T):
        self.sri = H
    
class SddsAttachDetachCB():
    
    def __init__(self):
        self.attaches = []
        self.detaches = []
        
    def attach_cb(self, streamDef, user_id):
        print '********* Received attach from: %s ********' % user_id
        self.attaches.append(streamDef)
    def detach_cb(self, attachId):
        print 'Received detach of: %s' % attachId
        self.detaches.append(attachId)
    def get_attach(self):
        return self.attaches
    def get_detach(self):
        return self.detaches
        

# TODO: I changed the inheritance from RHTest to ScaComponentTestCase, put back after figuring out how to deal with
# the new type
class ComponentTests(ossie.utils.testing.ScaComponentTestCase):
    # Path to the SPD file, relative to this file. This must be set in order to
    # launch the component.
    SPD_FILE = '../SinkSDDS.spd.xml'

# TODO: Standard Format check
# TODO: Original Format check
# TODO: Spectral Sense check
# TODO: df/dt drift test
# TODO: Time code valid checks
# TODO: Track down issue where RH cannot release a waveform, I believe it has to do with releasing the component when a stream is running
# TODO: Write unit test for timing checks
# TODO: Create property for overwriting SRI
# TODO: SDDS Override


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
            

# TODO
#     def testTimeDrift(self):
#         self.octetConnect()
#         sb.start()
#         fakeData = 100*[10];
#         for x in range(100):
#             self.sendPacket(fakeData, 1.0, True, eos=False)
#         
#         time.sleep(1)

    def testDfdtOverride(self):
        self.octetConnect()
        sink = sb.DataSinkSDDS()
        self.comp.override_sdds_header.enabled = True
        
        for dfdt in [x * .1 for x in range(10)]:
            time.sleep(0.1)
            self.comp.override_sdds_header.dfdt = dfdt
            sb.start()
            fakeData = 1024*[1];
            self.source.push(fakeData, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
            rcv = self.getPacket()
            sdds_header = self.getHeader(rcv)
            self.assertEqual(sdds_header.dfdt, int(1073741824*dfdt), "Received MSdelta that did not match expected")
            sb.stop()

    def testMsdelOverride(self):
        self.octetConnect()
        sink = sb.DataSinkSDDS()
        self.comp.override_sdds_header.enabled = True
        
        for i in range(10):
            msdel = random.randint(0,65535)
            time.sleep(0.1)
            self.comp.override_sdds_header.msdel = msdel
            sb.start()
            fakeData = 1024*[1];
            self.source.push(fakeData, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
            rcv = self.getPacket()
            sdds_header = self.getHeader(rcv)
            self.assertEqual(sdds_header.timeCode1msDelta, msdel, "Received MSdelta that did not match expected")
            sb.stop()
            
            
    def testMsptrOverride(self):
        self.octetConnect()
        sink = sb.DataSinkSDDS()
        self.comp.override_sdds_header.enabled = True
        
        for i in range(10):
            msptr = random.randint(0,2047)
            time.sleep(0.1)
            self.comp.override_sdds_header.msptr = msptr
            sb.start()
            fakeData = 1024*[1];
            self.source.push(fakeData, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
            rcv = self.getPacket()
            rcv_msptr = self.getMsptr(rcv)
            self.assertEqual(rcv_msptr, msptr, "Received msptr that did not match expected")
            sb.stop()

    def testSsvOverride(self):
        self.octetConnect()
        sink = sb.DataSinkSDDS()
        self.comp.override_sdds_header.enabled = True
        
        for ssv in range(2):
            time.sleep(0.1)
            self.comp.override_sdds_header.sscv = ssv
            sb.start()
            fakeData = 1024*[1];
            self.source.push(fakeData, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
            rcv = self.getPacket()
            sdds_header = self.getHeader(rcv)
            self.assertEqual(sdds_header.ssv, ssv, "Received SSV that did not match expected")
            sb.stop()
            
    def testTtvOverride(self):
        self.octetConnect()
        sink = sb.DataSinkSDDS()
        self.comp.override_sdds_header.enabled = True
        
        for ttv in range(2):
            time.sleep(0.1)
            self.comp.override_sdds_header.ttv = ttv
            sb.start()
            fakeData = 1024*[1];
            self.source.push(fakeData, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
            rcv = self.getPacket()
            sdds_header = self.getHeader(rcv)
            self.assertEqual(sdds_header.ttv, ttv, "Received TTV that did not match expected")
            sb.stop()

    def testTtvOverride(self):
        self.octetConnect()
        sink = sb.DataSinkSDDS()
        self.comp.override_sdds_header.enabled = True
        
        for ttv in range(2):
            time.sleep(0.1)
            self.comp.override_sdds_header.ttv = ttv
            sb.start()
            fakeData = 1024*[1];
            self.source.push(fakeData, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
            rcv = self.getPacket()
            sdds_header = self.getHeader(rcv)
            self.assertEqual(sdds_header.ttv, ttv, "Received TTV that did not match expected")
            sb.stop()
            
    def testMsvOverride(self):
        self.octetConnect()
        sink = sb.DataSinkSDDS()
        self.comp.override_sdds_header.enabled = True
        
        for msv in range(2):
            time.sleep(0.1)
            self.comp.override_sdds_header.msv = msv
            sb.start()
            fakeData = 1024*[1];
            self.source.push(fakeData, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
            rcv = self.getPacket()
            sdds_header = self.getHeader(rcv)
            self.assertEqual(sdds_header.msv, msv, "Received MSV that did not match expected")
            sb.stop()

    def testCxOverride(self):
        self.octetConnect()
        sink = sb.DataSinkSDDS()
        self.comp.override_sdds_header.enabled = True
        
        for cx in range(2):
            time.sleep(0.1)
            self.comp.override_sdds_header.cx = cx
            sb.start()
            fakeData = 1024*[1];
            self.source.push(fakeData, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
            rcv = self.getPacket()
            self.assertEqual(self.getCx(rcv), cx, "Received CX that did not match expected")
            sb.stop()
            
    def testBPSOverride(self):
        self.octetConnect()
        sink = sb.DataSinkSDDS()
        self.comp.override_sdds_header.enabled = True
        
        for bps in range(32):
            time.sleep(0.1)
            self.comp.override_sdds_header.bps = bps
            sb.start()
            fakeData = 1024*[1];
            self.source.push(fakeData, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
            rcv = self.getPacket()
            sdds_header = self.getHeader(rcv)
            self.assertEqual(bps, sum(sdds_header.bps), "Received BPS that did not match expected")
            sb.stop()

    def testEndianChange(self):
        self.octetConnect()
        sink = sb.DataSinkSDDS()
        sink._sink = SDDSSink2(sink) # Work around for framework bug
        sink._snk = sink._sink
        ad_cb = SddsAttachDetachCB();
        sink.registerAttachCallback(ad_cb.attach_cb)
        sink.registerDetachCallback(ad_cb.detach_cb)
        
        self.comp.connect(sink)
        self.comp.sdds_settings.endian_representation = 0 # Little Endian
        sb.start()
        
        fakeData = 1024*[1];
        self.source.push(fakeData, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
        time.sleep(1)
        print(sink.sri().keywords)
        
        # Check the keywords
        self.comp.stop()
        time.sleep(0.1)
        self.comp.sdds_settings.endian_representation = 1 # Big Endian
        self.comp.start()
        time.sleep(0.1)
        self.source.push(fakeData, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
        time.sleep(0.1)
        print(sink.sri().keywords)
        # Check the keywords

    def testStartStopStart(self):
        self.octetConnect()
        sink = sb.DataSinkSDDS()
        ad_cb = SddsAttachDetachCB();
        sink.registerAttachCallback(ad_cb.attach_cb)
        sink.registerDetachCallback(ad_cb.detach_cb)
        
        self.comp.connect(sink)
        sb.start()
        
        fakeData = 1024*[1];
        fakeData2 = 1024*[2];
        self.assertEqual(len(ad_cb.get_attach()), 0, "Should not have received any attaches")
        self.source.push(fakeData, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
        rcv = self.getPacket()[-1024:]
        self.assertEqual(fakeData, list(struct.unpack('1024B', rcv)))
        self.comp.stop()
        time.sleep(0.1)
        self.comp.start()
        time.sleep(0.1)
        self.source.push(fakeData2, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
        rcv = self.getPacket()[-1024:]
        self.assertEqual(fakeData2, list(struct.unpack('1024B', rcv)))
        
        
# TODO: The attach detach test do not pass, is it a bug with me or them?
    def testMultipleStreamsDifferentPort(self):
        self.octetConnect()
        
        short_source = sb.DataSource()
        short_source.connect(self.comp, usesPortName='shortOut')
        
        sink = sb.DataSinkSDDS()
        ad_cb = SddsAttachDetachCB();
        sink.registerAttachCallback(ad_cb.attach_cb)
        sink.registerDetachCallback(ad_cb.detach_cb)
        self.comp.connect(sink)
        sb.start()
        
        goodData1 = 1024*[1];
        badData = 512*[2];
        goodData2 = 512*[3];

        # No data pushed, no attaches or detaches        
        self.assertEqual(len(ad_cb.get_attach()), 0, "Should not have received any attaches")
#         self.assertEqual(len(ad_cb.get_detach()), 0, "Should not have received any detaches")
        
        # Push one good packet and confirm it was received
        self.source.push(goodData1, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
        self.assertEqual(goodData1, list(struct.unpack('1024B', self.getPacket()[-1024:])))
        # Since we pushed, we should get an attach, no detach
        self.assertEqual(len(ad_cb.get_attach()), 1, "Should have received 1 attach total")
#         self.assertEqual(len(ad_cb.get_detach()), 0, "Should not have received any detaches")
        
        # Push a new stream, it should get ignored, confirm we receive no data and still have only a single attach
        short_source.push(badData, EOS=False, streamID="Bad Stream", sampleRate=1.0, complexData=False, loop=False)
        self.assertEqual(len(self.getPacket()), 0, "Should not have passed on new stream, stream already active")
        self.assertEqual(len(ad_cb.get_attach()), 1, "Should have received 1 attach total")
#         self.assertEqual(len(ad_cb.get_detach()), 0, "Should not have received any detaches")

        # Push an EOS which should cause a detach        
        self.source.push(goodData1, EOS=True, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
        self.assertEqual(goodData1, list(struct.unpack('1024B', self.getPacket()[-1024:])))
        time.sleep(2)
        self.assertEqual(len(ad_cb.get_attach()), 1, "Should have received 1 attach total")
#         self.assertEqual(len(ad_cb.get_detach()), 1, "Should have received 1 detach total")

        # Send a new stream, which means a new attach                
        short_source.push(goodData2, EOS=False, streamID="New Stream", sampleRate=1.0, complexData=False, loop=False)
        self.assertEqual(goodData2, list(struct.unpack('!512H', self.getPacket()[-1024:])))
        self.assertEqual(len(ad_cb.get_attach()), 2, "Should have received 2 attach total")
#         self.assertEqual(len(ad_cb.get_detach()), 1, "Should have received 1 detach total")
        
        # Tear stuff down, confirm we get the final detach
        sb.release()
        self.assertEqual(len(ad_cb.get_attach()), 2, "Should have received 2 attach total")
        self.assertEqual(len(ad_cb.get_detach()), 2, "Should have received 2 detach total")

# TODO: The attach detach test do not pass, is it a bug with me or them?
    def testMultipleStreamsSamePort(self):
        self.octetConnect()
        sink = sb.DataSinkSDDS()
        ad_cb = SddsAttachDetachCB();
        sink.registerAttachCallback(ad_cb.attach_cb)
        sink.registerDetachCallback(ad_cb.detach_cb)
        self.comp.connect(sink)
        sb.start()
        
        goodData1 = 1024*[1];
        badData = 1024*[2];
        goodData2 = 1024*[3];

        # No data pushed, no attaches or detaches        
        self.assertEqual(len(ad_cb.get_attach()), 0, "Should not have received any attaches but we have: %s " % len(ad_cb.get_attach()))
#         self.assertEqual(len(ad_cb.get_detach()), 0, "Should not have received any detaches")
        
        # Push one good packet and confirm it was received
        self.source.push(goodData1, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
        self.assertEqual(goodData1, list(struct.unpack('1024B', self.getPacket()[-1024:])))
        # Since we pushed, we should get an attach, no detach
        self.assertEqual(len(ad_cb.get_attach()), 1, "Should have received 1 attach total")
#         self.assertEqual(len(ad_cb.get_detach()), 0, "Should not have received any detaches")
        
        # Push a new stream, it should get ignored, confirm we receive no data and still have only a single attach
        self.source.push(badData, EOS=False, streamID="Bad Stream", sampleRate=1.0, complexData=False, loop=False)
        self.assertEqual(len(self.getPacket()), 0, "Should not have passed on new stream, stream already active")
        self.assertEqual(len(ad_cb.get_attach()), 1, "Should have received 1 attach total")
#         self.assertEqual(len(ad_cb.get_detach()), 0, "Should not have received any detaches")

        # Push an EOS which should cause a detach        
        self.source.push(goodData1, EOS=True, streamID=self.id(), sampleRate=1.0, complexData=False, loop=False)
        self.assertEqual(goodData1, list(struct.unpack('1024B', self.getPacket()[-1024:])))
        time.sleep(2)
        self.assertEqual(len(ad_cb.get_attach()), 1, "Should have received 1 attach total")
#         self.assertEqual(len(ad_cb.get_detach()), 1, "Should have received 1 detach total")

        # Send a new stream, which means a new attach                
        self.source.push(goodData2, EOS=False, streamID="New Stream", sampleRate=1.0, complexData=False, loop=False)
        self.assertEqual(goodData2, list(struct.unpack('1024B', self.getPacket()[-1024:])))
        self.assertEqual(len(ad_cb.get_attach()), 2, "Should have received 2 attach total")
#         self.assertEqual(len(ad_cb.get_detach()), 1, "Should have received 1 detach total")
        
        # Tear stuff down, confirm we get the final detach
        sb.release()
        self.assertEqual(len(ad_cb.get_attach()), 2, "Should have received 2 attach total")
        self.assertEqual(len(ad_cb.get_detach()), 2, "Should have received 2 detach total")
        
    def testReleaseWhileStreaming(self):
        problem=False
        self.octetConnect()
        sink = sb.DataSinkSDDS()
        self.comp.connect(sink)
        sb.start()
        fakeData = 1024*[10];
        self.source.push(1000*fakeData, EOS=False, streamID=self.id(), sampleRate=1.0, complexData=False, loop=True)
        time.sleep(1)
        try:
            sb.release()
        except:
            e = sys.exc_info()[0]
            print(type(e.message))
            print("Unexpected error: %s", e)
            problem=True
        
        self.assertTrue(not problem, "exception was raised while releasing the component")
            
        
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
    
    def testRealLittleEndianShort(self):
        self.comp.sdds_settings.endian_representation=0
        self.dataShortInTest(False, little_endian=True)
    def testComplexLittleEndianShort(self):
        self.comp.sdds_settings.endian_representation=0
        self.dataShortInTest(True, little_endian=True)
        
    def testRealLittleEndianFloat(self):
        self.comp.sdds_settings.endian_representation=0
        self.dataFloatInTest(False, little_endian=True)
    def testComplexLittleEndianFloat(self):
        self.comp.sdds_settings.endian_representation=0
        self.dataFloatInTest(True, little_endian=True)
    
        
    def dataOctetInTest(self, is_complex):
        self.octetConnect()
        sb.start()
        
        # Create data
        fakeData = [x % 256 for x in range(1024)]
        recv = self.sendPacketGetPacket(fakeData, is_complex=is_complex, eos=True)
        
        # Validate correct amount of data was received
        self.assertEqual(len(recv), 1080)
        
        # Validate data is correct
        self.assertEqual(fakeData, list(struct.unpack('!1024B', recv[-1024:])))
        
        # Validate bps 
        sdds_header = self.getHeader(recv)
        self.assertEqual(sdds_header.bps, OCTET_BPS, "SDDS bps does not match expected.")

    def dataShortInTest(self, is_complex, little_endian=False):
        self.shortConnect()
        sb.start()
        
        # Create data
        fakeData = [x % 256 for x in range(1024/2)]
        recv = self.sendPacketGetPacket(fakeData, is_complex=is_complex, eos=True)
        
        # Validate correct amount of data was received
        self.assertEqual(len(recv), 1080)
        
        # Validate data is correct
        if little_endian:
            self.assertEqual(fakeData, list(struct.unpack('<512H', recv[-1024:])))
        else:
            self.assertEqual(fakeData, list(struct.unpack('!512H', recv[-1024:])))
        
        # Validate bps 
        sdds_header = self.getHeader(recv)
        self.assertEqual(sdds_header.bps, SHORT_BPS, "SDDS bps does not match expected.")

    def dataFloatInTest(self, is_complex, little_endian=False):
        self.floatConnect()
        sb.start()
        
        # Create data
        fakeData = [x % 256 for x in range(1024/4)]
        recv = self.sendPacketGetPacket(fakeData, is_complex=is_complex, eos=True)
        
        # Validate correct amount of data was received
        self.assertEqual(len(recv), 1080)
        
        # Validate data is correct
        if little_endian:
            self.assertEqual(fakeData, list(struct.unpack('<256f', recv[-1024:])))
        else:
            self.assertEqual(fakeData, list(struct.unpack('!256f', recv[-1024:])))
        
        # Validate bps 
        sdds_header = self.getHeader(recv)
        self.assertEqual(sdds_header.bps, FLOAT_BPS, "SDDS bps does not match expected.")
        
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
        
        MSV_MASK = (1 << 7 + 8)
        TTV_MASK = (1 << 6 + 8)
        SSV_MASK = (1 << 5 + 8)
        
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
        
        MSV = (raw_header[2] & MSV_MASK) >> 7 + 8
        TTV = (raw_header[2] & TTV_MASK) >> 6 + 8
        SSV = (raw_header[2] & SSV_MASK) >> 5 + 8
        
        DFDT = raw_header[6]
        FREQ = raw_header[7]
        
        # TODO: Fix these by shifting them down
        MSD = raw_header[3]
        TT = raw_header[4]
        TTE = raw_header[5]
         
        return Sdds.SddsHeader(FSN, SF, SoS, PP, OF, SS, DM, BPS, MSV, TTV, SSV, MSD, TT, TTE, DFDT, FREQ)

    def getCx(self, p):
        CX_MASK = (1 << (0 + 7))
        raw_header = struct.unpack('>HHHHQLLQ24B', p[:56])
        return (raw_header[0] & CX_MASK) >> 7
    def getMsptr(self,p):
        MSPTR_MASK = 8191
        raw_header = struct.unpack('>HHHHQLLQ24B', p[:56])
        return (raw_header[2] & MSPTR_MASK)
        

if __name__ == "__main__":
    ossie.utils.testing.main("../SinkSDDS.spd.xml") # By default tests all implementations
