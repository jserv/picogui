#!/usr/bin/env python
"""
 A client for the announceBot that accepts email messages from stdin.
 This is meant to be used from a .forward file.
 The message should have a subject of the form "Announce <channel>".
 Every non-blank line of the body will be sent as a message to the bot.
"""
from twisted.internet import reactor, protocol
import sys, email

logFile = "/home/commits/mail.log"
socketName = "/tmp/announceBot.socket"
allowedCommands = ("Announce", "JoinChannel", "PartChannel")

class AnnounceClient(protocol.Protocol):
    def connectionMade(self):
        import sys
        mailMsg  = email.message_from_file(sys.stdin)
        f = open(logFile, "a")
	f.write(mailMsg.as_string())
	f.close()
        subjectFields = mailMsg['Subject'].split(" ")
        messages = mailMsg.get_payload().split("\n")
        if subjectFields[0] in allowedCommands:
	    # We want to send at least one command, even if the body is empty.
	    if not messages:
	        messages = [" "]
            for line in messages:
	    	line = line.strip()
		if len(line) > 0:
	            self.transport.write("%s %s %s\r\n" %
                                         (subjectFields[0], subjectFields[1], line))
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
