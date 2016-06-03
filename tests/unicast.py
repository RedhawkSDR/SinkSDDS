#!/usr/bin/python
    
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
