void channel_msg (struct server *serv, char *outbuf, char *chan, char *from,
						char *text, char fromme);
void clear_channel (struct session *sess);
void set_topic(struct session *sess, char *topic);
void private_msg (struct server *serv, char *tbuf, char *from, char *ip,
				 char *text);
void channel_action (struct session *sess, char *tbuf, char *chan, char *from,
					 char *text, int fromme);
void user_new_nick (struct server *serv, char *nick, char *newnick, int quiet);
void set_server_name (struct server *serv, char *name);
void do_dns (struct session *sess, char *tbuf, char *nick, char *host);
void process_line (struct server *serv, char *buf);

