import sys
import XChat

def signalhandler(name, flags, args):
          session = args[0].get_session()
  	  server = args[1].get_server()
  	  message = args[2].get_string()

	  print "Message to " + session.info()['channel'] + " from " + server.info()['hostname'] + ": " + message

i = XChat.XChat ()
i.register ("test", "test")
i.hook_signal ("XP_INBOUND", signalhandler)
