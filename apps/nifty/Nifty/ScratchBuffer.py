from Buffer import Buffer

class ScratchBuffer(Buffer):
    "A buffer to hold temporary stuff"

    default_name = '__scratch__'

    def save(self):
        print 'cannot save a ScratchBuffer'
