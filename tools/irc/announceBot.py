#!/usr/bin/env python
"""
A quick and dirty twisted.im-based announce bot. It simply says anything it reads
from clients connecting to it on a UNIX socket
"""
socketName = "/tmp/announceBot.socket"


from twisted.im import basechat, baseaccount, ircsupport 
from twisted.internet.protocol import Factory
from twisted.internet.app import Application
from twisted.protocols.basic import LineReceiver
import time

accounts = [
    ircsupport.IRCAccount("IRC", 1,
        # Lalo's joke: A brainless entity created to keep an eye on subversion                 
	"CIA",              # nickname
        "",                 # passwd
        "irc.freenode.net", # irc server
        6667,               # port
        "picogui, commits", # comma-seperated list of channels
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
	fields = line.split(" ", 2)
	if fields[0] == "Announce":
	    try:
	        groups['commits'].sendText(fields[1] + ": " + fields[2])
	        groups[fields[1]].sendText(fields[2])
		time.sleep(1)
            except KeyError:
	        pass
        elif fields[0] == "JoinChannel":
            accounts[0].client.join(fields[1])
        elif fields[0] == "PartChannel":
            accounts[0].client.leave(fields[1])

class BotConversation(basechat.Conversation):
    pass

class BotGroupConversation(basechat.GroupConversation):
    def show(self):
        global groups
        groups[self.group.name] = self
    
class BotChat(basechat.ChatUI):
    def getGroupConversation(self, group, Class=BotGroupConversation, stayHidden=0):
        return basechat.ChatUI.getGroupConversation(self, group, Class, 
            stayHidden)

    def getConversation(self, person, Class=BotConversation, stayHidden=0):
        return basechat.ChatUI.getConversation(self, person, Class, stayHidden)

if __name__ == "__main__":
    from twisted.internet import reactor
    AccountManager()
    serv = Factory()
    serv.protocol = AnnounceServer
    reactor.listenUNIX(socketName, serv)
    
    reactor.run()

