import PicoGUI
app = PicoGUI.InvisibleApp()
app.server.dup(app.server.mktemplate(open(sys.argv[1]).read()))
app.run()
