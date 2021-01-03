import unittest
import yade.libVersions

class TestLibVersions(unittest.TestCase):
	def setUp(self):
		pass # no setup needed for tests here
	def testLibVersions(self):
		# calling them is enough, to make sure they do not throw any errors.
		cmakeVer = yade.libVersions.getAllVersionsCmake()
		cppVer   = yade.libVersions.getAllVersionsCpp()
		print("cmake versions: ", cmakeVer)
		print("C++ versions: ", cppVer)
		print("\n")
		yade.libVersions.printAllVersions(True)
		yade.libVersions.printAllVersions(False)
		for key,val in cmakeVer.items():
			if((key in cppVer) and (len(val)==2) and (len(cppVer[key])==2)):
				print(str(key)+" version reported by by cmake is ",val," and by C++ is ",cppVer[key])
				if (val[0] != cppVer[key][0]):
					print('\n\033[93m'+" Warning: "+'\033[0m'+str(key)+" versions are different, CMAKE: "+str(val[0])+" vs. C++: "+str(cppVer[key][0]))
					print(" Something suspicious is going on. Can you help with file py/libVersions.py.in?")
