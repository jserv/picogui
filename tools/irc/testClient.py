#!/usr/bin/env python
"""
  Test client that just spews any command line args at the socket
"""
from twisted.internet import reactor, protocol

socketName = "/tmp/announceBot.socket.1"

class AnnounceClient(protocol.Protocol):
    def connectionMade(self):
        import sys
	self.transport.write(" ".join(sys.argv[1:]) + "\r\n")
        self.transport.loseConnection()
    
    def connectionLost(self, reason):
        from twisted.internet import reactor
        reactor.stop()

class AnnounceClientFactory(protocol.ClientFactory):
    protocol = AnnounceClient

    def clientConnectionFailed(self, connector, reason):
        reactor.stop()
    
    def clientConnectionLost(self, connector, reason):
        reactor.stop()

if __name__ == '__main__':
    import sys
    f = AnnounceClientFactory()
    reactor.connectUNIX(socketName, f)
    reactor.run()
