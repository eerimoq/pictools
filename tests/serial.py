from unittest.mock import Mock


class Serial(object):

    __init__ = Mock(return_value=None)
    write = Mock()
    read = Mock()
    close = Mock()

    @classmethod
    def reset_mock(cls):
        cls.__init__.reset_mock()
        cls.write.reset_mock()
        cls.read.reset_mock()
        cls.close.reset_mock()
