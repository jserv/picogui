#!/usr/bin/env python
"""
 A quick and dirty client for the annouceBot. Usage:
   announceClient.py server channel message
"""

from twisted.internet import reactor, protocol

class AnnounceClient(protocol.Protocol):
    def connectionMade(self):
        import sys
        self.transport.write("Announce %s %s\r\n" % (sys.argv[2], " ".join(sys.argv[3:]).split('\n')[0]))
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
    reactor.connectTCP(sys.argv[1], 30400, f)
    reactor.run()
