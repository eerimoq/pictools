from unittest.mock import Mock


class Serial(object):

    __init__ = Mock()

    write = Mock()

    read = Mock()

    @classmethod
    def reset_mock(cls):
        cls.__init__.reset_mock()
        cls.write.reset_mock()
        cls.read.reset_mock()
