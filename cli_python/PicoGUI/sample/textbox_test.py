import PicoGUI
app = PicoGUI.Application('Textbox Test')
tb = app.addWidget('Textbox')
tb.side = 'all'
app.addWidget('Scroll').bind = tb
tb.text = """Beastie Boys - Just a Test

Like a flag at half mast as frames click fast
Not a thing will last as past is past
Like stacks of thoughts that got played and worn
Used over and over till they were tired and torn
Like a broken clock that can't tell time
Like a thick ass book that's filled with wack rhymes
Like a scorching blaze that burned the sand
Like a band that planned and planned and planned
And flew down like a raven in the dark of night
And snatched up the worm helpless to fight
And brought it back to the nest singing microphone check
One two one two this is just a test

Like sand in one hand You can't hold for long
Like scheming on a plan that goes all wrong
Like fudge and caramel they're not the same
And it's a shame all you talk is game
You act like a diamond waiting to be set
In a gold ring, as if, I bet
As time goes by in this give and take
As long as I learn I will make mistakes
What do I want? What do I need?
Why do I want it? What's in it for me?
It's the imagery of technology
Is what you get is what you see
Don't worry your mind
When you give it your best
One two one two this is just a test"""

app.run()
print "Testing text readback:\n\n\"" + app.server.getstring(tb.text)[:-1] + "\"\n";
