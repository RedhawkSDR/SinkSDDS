#!/usr/bin/env python

import ossie.utils.testing
from ossie.utils import sb
import unicast
import time
import socket
import struct

DEBUG_LEVEL = 3
UNICAST_PORT = 1234
UNICAST_IP = '127.0.0.1'

# TODO: I changed the inheritance from RHTest to ScaComponentTestCase, put back after figuring out how to deal with
# the new type
class ComponentTests(ossie.utils.testing.ScaComponentTestCase):
    # Path to the SPD file, relative to this file. This must be set in order to
    # launch the component.
    SPD_FILE = '../SinkSDDS.spd.xml'

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
        recv = self.sendPacketGetPacket(fakeData, is_complex=is_complex)
        
        # Validate correct amount of data was received
        self.assertEqual(len(recv), 1080)
        
        # Validate data is correct
        self.assertEqual(fakeData, list(struct.unpack('1024B', recv[-1024:])))

    def dataShortInTest(self, is_complex):
        self.shortConnect()
        sb.start()
        
        # Create data
        fakeData = [x % 256 for x in range(1024/2)]
        recv = self.sendPacketGetPacket(fakeData, is_complex=is_complex)
        
        # Validate correct amount of data was received
        self.assertEqual(len(recv), 1080)
        
        # Validate data is correct
        self.assertEqual(fakeData, list(struct.unpack('512H', recv[-1024:])))

    def dataFloatInTest(self, is_complex):
        self.floatConnect()
        sb.start()
        
        # Create data
        fakeData = [x % 256 for x in range(1024/4)]
        recv = self.sendPacketGetPacket(fakeData, is_complex=is_complex)
        
        # Validate correct amount of data was received
        self.assertEqual(len(recv), 1080)
        
        # Validate data is correct
        self.assertEqual(fakeData, list(struct.unpack('256f', recv[-1024:])))
        
    def sendPacketGetPacket(self, data_to_send, sample_rate=1.0, is_complex=False, socket_read_size=1080):
        self.sendPacket(data_to_send, sample_rate, is_complex)
        # Wait for data
        time.sleep(0.1)
        return self.getPacket(socket_read_size)
        
    def sendPacket(self, data_to_send, sample_rate=1.0, is_complex=False):
        self.source.push(data_to_send, False, self.id(), sample_rate, is_complex)
        
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

if __name__ == "__main__":
    ossie.utils.testing.main("../SinkSDDS.spd.xml") # By default tests all implementations
