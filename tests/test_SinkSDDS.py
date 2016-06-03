#!/usr/bin/env python

import ossie.utils.testing
from ossie.utils import sb
import unicast
import time
import socket
import struct

DEBUG_LEVEL = 5
UNICAST_PORT = 1234
UNICAST_IP = '127.0.0.1'

# TODO: I changed the inheritance from RHTest to ScaComponentTestCase, put back after figuring out how to deal with
# the new type
class ComponentTests(ossie.utils.testing.ScaComponentTestCase):
    # Path to the SPD file, relative to this file. This must be set in order to
    # launch the component.
    SPD_FILE = '../SinkSDDS.spd.xml'


    def testDataOctetIn(self):
        self.comp.start()
        
        # Create data
        fakeData = [x % 256 for x in range(1024)]
        recv = self.sendPacketGetPacket(fakeData, 'dataOctetIn')
        
        # Validate correct amount of data was received
        self.assertEqual(len(recv), 1080)
        
        # Validate data is correct
        self.assertEqual(fakeData, list(struct.unpack('1024B', recv[-1024:])))

    def testDataShortIn(self):
        self.comp.start()
        
        # Create data
        fakeData = [x % 256 for x in range(1024/2)]
        recv = self.sendPacketGetPacket(fakeData, 'dataShortIn')
        
        # Validate correct amount of data was received
        self.assertEqual(len(recv), 1080)
        
        # Validate data is correct
        self.assertEqual(fakeData, list(struct.unpack('512H', recv[-1024:])))

    def testDataFloatIn(self):
        self.comp.start()
        
        # Create data
        fakeData = [x % 256 for x in range(1024/4)]
        recv = self.sendPacketGetPacket(fakeData, 'dataFloatIn')
        
        # Validate correct amount of data was received
        self.assertEqual(len(recv), 1080)
        
        # Validate data is correct
        self.assertEqual(fakeData, list(struct.unpack('256f', recv[-1024:])))
        
    def sendPacketGetPacket(self, data_to_send, port_name, sample_rate=1.0):
        # Create source and connect it to component
        source = sb.DataSource();
        source.connect(self.comp, providesPortName=port_name)
        source.start()
        
        source.push(data_to_send, False, self.id(), sample_rate, False)
        
                # Wait for data
        time.sleep(0.1)
        
        # Get data
        received_data = []
        try:
             received_data = self.uclient.receive(1080)
        except socket.error as e:
            print "Socket read error: ", e
        
        return received_data

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

if __name__ == "__main__":
    ossie.utils.testing.main("../SinkSDDS.spd.xml") # By default tests all implementations
