from Buffer import TextBuffer

class ScratchBuffer(TextBuffer):
    "A buffer to hold temporary stuff"

    default_name = '__scratch__'

    def save(self):
        print 'cannot save a ScratchBuffer'
