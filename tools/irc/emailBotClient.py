#!/usr/bin/env python
"""
 A client for the announceBot that accepts email messages from stdin.
 This is meant to be used from a .forward file.
 The message should have a subject of the form "Announce <channel>".
 Every non-blank line of the body will be sent as a message to the bot.
"""
from twisted.internet import reactor, protocol
import sys, email, os

logFile = "/home/commits/mail.log"
statsDir = "/home/commits/stats"
statsSubdirs = ("forever", "daily", "weekly", "monthly")
socketName = "/tmp/announceBot.socket"

# Allowed commands, split up into those with content and those without
allowedTextCommands = ("Announce",)
allowedControlCommands = ("JoinChannel", "PartChannel")

# Prohibited channels
badChannels = ("shell","123")


def incrementProjectCommits(project):
    if project.find(os.sep) >= 0:
        return
    for statsSubdir in statsSubdirs:
        statFile = os.path.join(statsDir, statsSubdir, project)
        count = 0
        try:
            f = open(statFile)
            count = int(f.read().strip())
            f.close()
        except:
            pass
        count += 1
        f = open(statFile, "w")
        f.write("%d\n" % count)
        f.close()

class AnnounceClient(protocol.Protocol):
    def connectionMade(self):
        import sys
        mailMsg  = email.message_from_file(sys.stdin)
        f = open(logFile, "a")
	f.write(mailMsg.as_string())
	f.close()
        subjectFields = mailMsg['Subject'].split(" ")
        # This limits the length of the maximum message, mainly to prevent DOS'ing the bot too badly
        messages = mailMsg.get_payload().split("\n")[:40]
        subjectFields[1] = subjectFields[1].lower()
        if subjectFields[1][0] == "#":
            subjectFields[1] = subjectFields[1:]

        if not subjectFields[1] in badChannels:

            # Send allowed text commands
            if subjectFields[0] in allowedTextCommands:
                incrementProjectCommits(subjectFields[1])
                for line in messages:
                    line = line.strip()
                    if len(line) > 0:
                        self.transport.write("%s %s %s\r\n" %
                                             (subjectFields[0], subjectFields[1], line))

            # Send allowed control commands
            if subjectFields[0] in allowedControlCommands:
                self.transport.write("%s %s\r\n" % (subjectFields[0], subjectFields[1]))
            
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
