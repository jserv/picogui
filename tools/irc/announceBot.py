#!/usr/bin/env python
"""
A quick and dirty twisted.im based announce bot. It simply says anything it reads
from clients connecting to it on a UNIX socket
"""

socketName = "/tmp/announceBot.socket"
channelFile = "/home/commits/channels.list"

secondsBetweenLines = 0.5

from twisted.im import basechat, baseaccount, ircsupport 
from twisted.internet.protocol import Factory
from twisted.internet.app import Application
from twisted.internet import reactor
from twisted.protocols.basic import LineReceiver
from twisted.python import log
import time, irc_colors, glob, sys, re, os

# Lalo's joke: A brainless entity created to keep an eye on subversion                 
botNick = "CIA"

# List of channels we're in. These will be autojoined by the
# AccountManager. We update this and save it when we get a mail
# for joining or parting a channel.
print channelFile
f = open(channelFile)
channelList = {}
for line in f.readlines():
    line = line.strip()
    if line:
        channelList[line] = 1
f.close()

print channelList.keys();

# load the freenode password
password = ""
try:
    f = open("/home/commits/.fnpass")
    password = f.readline().rstrip()
except:
    pass

connected = 0 # false

accounts = [
    ircsupport.IRCAccount("IRC", 1,
        botNick,               # nickname
        password,              # passwd
        "irc.freenode.net",    # irc server
        6667,                  # port
        ""
    )
]

groups = {}

class AccountManager (baseaccount.AccountManager):
    def __init__(self):
        self.chatui = BotChat()
        if len(accounts) == 0:
            print "You have defined no accounts."
        for acct in accounts:
            defer = acct.logOn(self.chatui)
            defer.addCallback(self.logonCallback)
            defer.addErrback(log.err)

    def errback(self, acct):
        print "Error! "

    def logonCallback(self, acct):
        print "Logged on OK"
        time.sleep(4) # so it won't count towards our limit as much
        if password != "":
            print "Sending OPER"
            accounts[0].client.sendLine("OPER "+botNick+" "+password)  # identify to freenode
            print "Sending SILENCE line"
            accounts[0].client.sendLine("SILENCE +*@*") # as we don't accept commands via IRC, silence incoming messages to make it more DoS resistant

        for chan in channelList:
            print "Joining "+chan
            accounts[0].client.join(chan)

        connected = 1 # true
        print "Done"


class AnnounceServer(LineReceiver):
    def lineReceived(self, line):
        global groups
        try:
            (command, project, message) = line.split(" ", 2)
        except ValueError:
            (command, project) = line.split(" ", 2)

        project = project.replace("-ports", "-src") # freebsd specific hack
	if command == "Announce":
            # if we are the first bot, we send to the main channels
            if botID == 1:
                # Now we'll try to send the message to #commits, #only-commits, #<project>, and #<project>-commits.
                # No big deal if any of them fails becase we're not joined to that channel.
                try:
                    groups['only-commits'].sendText(irc_colors.boldify(project + ": ") + message)
                except KeyError:
                    pass
                try:
                    groups['commits'].sendText(irc_colors.boldify(project + ": ") + message)
                except KeyError:
                    pass
                
            try:
                groups[project].sendText(message)
            except KeyError:
	        pass
            try:
                groups[project+"-commits"].sendText(message)
            except KeyError:
	        pass
            time.sleep(secondsBetweenLines)

        elif command == "SendToChannels":
            # Send a message to a comma-separated list of channels.
            for channel in project.split(','):
                try:
                    groups[channel].sendText(message)
                except KeyError:
                    pass
            time.sleep(secondsBetweenLines)
            
        elif command == "JoinChannel":
            accounts[0].client.join(project)
            channelList[project] = 1

        elif command == "PartChannel":
            accounts[0].client.leave(project)
            try:
                del channelList[project]
            except KeyError:
                pass

        f = open(channelFile, "w")
        f.write("\n".join(channelList.keys()) + "\n")
        f.close()

class BotConversation(basechat.Conversation):
    def show(self):
        pass
    
    def hide(self):
        pass
    
    def showMessage(self, text, metadata=None):
        pass    

class BotGroupConversation(basechat.GroupConversation):
    def show(self):
        global groups
        groups[self.group.name] = self

    def hide(self):
        pass
    
    def showGroupMessage(self, sender, text, metadata=None):
        pass
    
    def setTopic(self, topic, author):
        pass
                                                       
class BotChat(basechat.ChatUI):
    def getGroupConversation(self, group, Class=BotGroupConversation, stayHidden=0):
        return basechat.ChatUI.getGroupConversation(self, group, Class, 
            stayHidden)

    def getConversation(self, person, Class=BotConversation, stayHidden=0):
        return basechat.ChatUI.getConversation(self, person, Class, stayHidden)

    def unregisterAccountClient(self, client):
        basechat.ChatUI.unregisterAccountClient(self, client)
        if connected:
            accounts[0].logOn(self)
        else:
            reactor.stop()

from twisted.internet.app import Application
application = Application("announceBot")
AccountManager()
serv = Factory()
serv.protocol = AnnounceServer
application.listenUNIX(socketName, serv)

if __name__ == '__main__':
    application.run()
