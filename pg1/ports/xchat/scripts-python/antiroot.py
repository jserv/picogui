import XChat
import string

def join_handler(name, flags, args):
	serv = args[0].get_server()
	chan = args[1].get_string()
	nick = args[2].get_string()
	ip = args[3].get_string()
	if string.find (ip, "root@") != -1:
		try:
			sess = XChat.Session()
			sess.set(serv.info()['hostname'], serv.info()['port'], serv.info()['nick'], chan)
		except LookupError:
			i.get_current_session().print_text("anti-root: Internal error")
			return
	sess.handle_cmd (nick + ": Don't IRC as root!")
	return

i = XChat.XChat()
i.register ("anti-root", "Send a message to someone when they join as *!root@*")
i.hook_signal ("XP_JOIN", join_handler)
