# NOTE: remember to add new tests to file py/tests/__init__.py

import unittest

class TestDummy(unittest.TestCase):
	def setUp(self):
		# print("-------------------> Running setUp") # uncomment for debugging
		pass # here prepare some internal variables used by this test
	def tearDown(self):
		# print("-------------------> Running tearDown") # uncomment for debugging
		pass # here finalize the tests, maybe close an opened file, or something else
	def thisTestsException(self):
		# return # uncommenting this line will cause test to fail
		raise RuntimeError("this tests exception")
	def testDummySomething(self):
		# print("-------------------> Running testDummySomething") # uncomment for debugging
		# test can check if something raises exception in case of problems
		self.assertRaises(RuntimeError,self.thisTestsException)
	def testDummySomethingElse(self):
		# print("-------------------> Running testDummySomethingElse") # uncomment for debugging
		# test also can check via assertEqual, assertTrue , assertFalse
		self.assertTrue(True)
		self.assertFalse(False)
		self.assertEqual(0,0)
		# https://docs.python.org/3/library/unittest.html
