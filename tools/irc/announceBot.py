#!/usr/bin/env python
"""
A quick and dirty twisted.im-based announce bot. It simply says anything it reads
from clients connecting to it on port 30400
"""

from twisted.im import basechat, baseaccount, ircsupport 
from twisted.internet.protocol import Factory
from twisted.internet.app import Application
from twisted.protocols.basic import LineReceiver

accounts = [
    ircsupport.IRCAccount("IRC", 1,
        "pgAnnounceBot",    # nickname
        "",                 # passwd
        "irc.freenode.net", # irc server
        6667,               # port
        "picogui",          # comma-seperated list of channels
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
	        groups[fields[1]].sendText(fields[2])
	    except KeyError:
	        pass

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
    reactor.listenTCP(30400, serv)
    
    reactor.run()

