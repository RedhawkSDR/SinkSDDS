#!/usr/bin/python
#
# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of REDHAWK rh.SinkSDDS.
#
# REDHAWK rh.SinkSDDS is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# REDHAWK rh.SinkSDDS is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see http://www.gnu.org/licenses/.
#
    
import socket
import struct
import sys
import time

class unicast_server(object):
    def __init__(self, addr, port, ttl=64):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.setsockopt(socket.IPPROTO_IP, socket.IP_TTL, ttl)
        
        self.addr = addr
        self.port = port
        
    def send(self, data):
        bytessent = self.sock.sendto(data, (self.addr, self.port))
        if bytessent != len(data):
            print "Failure sending all bytes"
            
class unicast_client(object):
    def __init__(self, addr, port):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((addr, port))
        
    def receive(self, bytesToReceive=10240, timeout = 1):
        self.sock.settimeout(timeout)
        recvData = self.sock.recv(bytesToReceive)
        return recvData
