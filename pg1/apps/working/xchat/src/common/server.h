void connect_server (session * sess, char *hostname, int port, int no_login);
void disconnect_server (session * sess, int sendquit, int err);
int server_cleanup (server * serv);
void flush_server_queue (server *serv);
void auto_reconnect (server *serv, int send_quit, int err);
