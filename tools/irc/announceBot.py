#!/usr/bin/env python
"""
A quick and dirty twisted.im based announce bot. It simply says anything it reads
from clients connecting to it on a UNIX socket
"""
socketName = "/tmp/announceBot.socket"
channelFile = "/home/commits/channels.list"

from twisted.im import basechat, baseaccount, ircsupport 
from twisted.internet.protocol import Factory
from twisted.internet.app import Application
from twisted.protocols.basic import LineReceiver
import time, irc_colors, sys

# List of channels we're in. These will be autojoined by the
# AccountManager. We update this and save it when we get a mail
# for joining or parting a channel.
f = open(channelFile)
channelList = {}
for line in f.readlines():
    line = line.strip()
    if line:
        channelList[line] = 1
f.close()

accounts = [
    ircsupport.IRCAccount("IRC", 1,
        # Lalo's joke: A brainless entity created to keep an eye on subversion                 
        "CIA",              # nickname
        "",                 # passwd
        "irc.freenode.net", # irc server
        6667,               # port
        ",".join(channelList.keys())
    )
]

groups = {}

class AccountManager (baseaccount.AccountManager):
    def __init__(self):
        self.chatui = BotChat()
        if len(accounts) == 0:
            print "You have defined no accounts."
        for acct in accounts:
            acct.logOn(self.chatui)

class AnnounceServer(LineReceiver):
    def lineReceived(self, line):
        global groups
        try:
            (command, project, message) = line.split(" ", 2)
        except ValueError:
            (command, project) = line.split(" ", 2)
        
	if command == "Announce":
            # Now we'll try to send the message to #commits, #<project>, and #<project>-commits.
            # No big deal if any of them fails becase we're not joined to that channel.
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
            time.sleep(1)

        elif command == "SendToChannels":
            # Send a message to a comma-separated list of channels.
            for channel in project.split(','):
                try:
                    groups[channel].sendText(message)
                except KeyError:
                    pass
            
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

from twisted.internet.app import Application
application = Application("announceBot")
AccountManager()
serv = Factory()
serv.protocol = AnnounceServer
application.listenUNIX(socketName, serv)

if __name__ == '__main__':
    application.run()

