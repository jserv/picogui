#!/usr/bin/env python
"""
 A hackish little client for the announceBot that accepts email messages from stdin.
 This is meant to be used from a .forward file.
 The message should have a subject of the form "Announce <channel>".
 Every non-blank line of the body will be sent as a message to the bot.
 There's also a lot of goop in here to update stats.
"""
from twisted.internet import reactor, protocol
import sys, email, os
import irc_colors

logFile = "/home/commits/mail.log"
statsDir = "/home/commits/stats"
mtbcSubdir = "mtbc"
statsSubdirs = ("forever", "daily", "weekly", "monthly")
socketName = "/tmp/announceBot.socket"
import re, time

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
               "\247\247\247",

               # Commits is in here, since the bot is in it by
               # default and shouldn't be able to leave
               "commits",
               )

# Little utilities to save and load numbers from files... for our cheesy stats directory
def saveInt(f, i):
    f = open(f, "w")
    f.write("%d\n" % i)
    f.close()

def loadInt(f):
    f = open(f)
    count = int(f.read().strip())
    f.close()
    return count

def addIntToFile(f, i):
    try:
        count = loadInt(f)
    except IOError:
        count = None
    if not count:
        count = 0
    count += i
    saveInt(f, count)

def incrementProjectCommits(project):
    if project.find(os.sep) >= 0:
        return
    for statsSubdir in statsSubdirs:
        addIntToFile(os.path.join(statsDir, statsSubdir, project), 1)

def updateCommitTimes(project):
    """Record data for the Mean Time Between Commits page"""
    currentTime = time.time()
    lastTime = None
    try:
        lastTime = loadInt(os.path.join(statsDir, mtbcSubdir, project + '.lastTime'))
    except IOError:
        pass
    if lastTime:
        addIntToFile(os.path.join(statsDir, mtbcSubdir, project + '.numSamples'), 1)
        addIntToFile(os.path.join(statsDir, mtbcSubdir, project + '.totalTime'), currentTime - lastTime)
    saveInt(os.path.join(statsDir, mtbcSubdir, project + '.lastTime'), currentTime)

def updateStats(project):
    incrementProjectCommits(project)
    updateCommitTimes(project)

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

        # Don't allow known bad channels, or channel names with slashes
        if not subjectFields[1] in badChannels and subjectFields[1].find(os.sep) < 0:

            # Send allowed text commands
            if subjectFields[0] in allowedTextCommands:
                # Our lame little stat page
                updateStats(subjectFields[1])
                
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
