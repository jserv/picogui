#!/usr/bin/env python
"""
 A client for the announceBot that accepts email messages from stdin.
 This is meant to be used from a .forward file.
 The message should have a subject of the form "Announce <channel>".
 Every non-blank line of the body will be sent as a message to the bot.
"""
from twisted.internet import reactor, protocol
import sys, email, os
import irc_colors

logFile = "/home/commits/mail.log"
statsDir = "/home/commits/stats"
statsSubdirs = ("forever", "daily", "weekly", "monthly")
socketName = "/tmp/announceBot.socket"
import re

# Allowed commands, split up into those with content and those without
allowedTextCommands = ("Announce",)
allowedControlCommands = ("JoinChannel", "PartChannel")

# Prohibited channels
# List from http://www.freenode.net/drones.shtml
badChannels = ("#!~!raisin!!",
               "0-xdcc!",
               "03337",
               "123",
               "acs45",
               "conscriptp",
               "efferz",
               "hackers",
               "infected",
               "livethisgame",
               "nodo747",
               "nonsense",
               "plazateam",
               "r3p4d",
               "scan",
               "secrets",
               "shell",
               "soviet-union",
               "techno-sound",
               "test12",
               "[alpha]",
               "\247\246\247\246\247",
               "\247\247\247")

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

def applyColorTags(message):
    # We support tags of the form {red}, {bold}, {normal}, etc.
    message = re.sub('{bold}', irc_colors.BOLD, message)
    message = re.sub('{normal}', irc_colors.NORMAL, message)
    message = re.sub('{reverse}', irc_colors.REVERSE, message)
    message = re.sub('{underline}', irc_colors.UNDERLINE, message)
    for color in irc_colors.COLORS:
        message = re.sub('{%s}' % color, irc_colors.COLOR_PREFIX + irc_colors.COLORS[color], message)
    return message

class AnnounceClient(protocol.Protocol):
    def connectionMade(self):
        import sys
        mailMsg  = email.message_from_file(sys.stdin)
        f = open(logFile, "a")
        f.write(mailMsg.as_string())
        f.close()
        subjectFields = mailMsg['Subject'].split(" ")
        message = mailMsg.get_payload()
        subjectFields[1] = subjectFields[1].lower()
        if subjectFields[1][0] == "#":
            subjectFields[1] = subjectFields[1][1:]

        message = applyColorTags(message)

        if not subjectFields[1] in badChannels:

            # Send allowed text commands
            if subjectFields[0] in allowedTextCommands:
                # Our lame little stat page
                incrementProjectCommits(subjectFields[1])
                
                # This limits the length of the maximum message, mainly to prevent DOS'ing the bot too badly
                for line in message.split("\n")[:40]:
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
