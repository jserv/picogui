from Buffer import Buffer

class ScratchBuffer(Buffer):
    "A buffer to hold temporary stuff"

    def save(self):
        print 'cannot save a ScratchBuffer'
