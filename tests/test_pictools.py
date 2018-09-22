import unittest

try:
    from unittest.mock import patch
except ImportError:
    from mock import patch

try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

import pictools


class PicToolsTest(unittest.TestCase):

    def test_vehicle(self):
        pass


if __name__ == '__main__':
    unittest.main()
