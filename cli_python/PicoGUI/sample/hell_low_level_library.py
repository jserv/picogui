# low-level library version of Hello, World
# missing a real event loop

from PicoGUI import network, requests, responses

def test(where='localhost'):
  connection = network.sock(where)
  pg_in = connection.makefile()
  connection.send(requests.mkstring('Greetings'))
  string_id = responses.next(pg_in)
  connection.send(requests.register(string_id))
  top_id = responses.next(pg_in)
  # PG_WIDGET_LABEL is 1
  connection.send(requests.createwidget(1))
  label_id = responses.next(pg_in)
  connection.send(requests.mkstring('Hello, World'))
  string_id = responses.next(pg_in)
  # PG_WP_TEXT is 7
  connection.send(requests.set(label_id, 7, string_id))
  responses.next(pg_in)
  # PG_WP_SIDE is 2, PG_S_ALL is 2048 (1<<11)
  connection.send(requests.set(label_id, 2, 2048))
  responses.next(pg_in)
  # PG_FSTYLE_BOLD is 256 (1<<8)
  connection.send(requests.mkfont('', 256, 24))
  font_id = responses.next(pg_in)
  # PG_WP_FONT is 8
  connection.send(requests.set(label_id, 8, font_id))
  responses.next(pg_in)
  # PG_DERIVE_INSIDE is 2
  connection.send(requests.attachwidget(top_id, label_id, 2))
  responses.next(pg_in)
  connection.send(requests.update())
  responses.next(pg_in)
  return connection

def event_loop(connection):
  pg_in = connection.makefile()
  while 1:
    connection.send(requests.wait())
    ev = responses.next(pg_in)
    if isinstance(ev, responses.Event) and ev.type == 3: # PG_WE_CLOSE
      return

if __name__ == '__main__':
	c = test()
        event_loop(c)