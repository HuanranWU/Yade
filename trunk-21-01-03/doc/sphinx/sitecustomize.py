# necessary for our docstrings containing unicode characters
import sys
try: #for python2
	sys.setdefaultencoding('utf8')
except:
	pass
