# encoding: utf-8
# 2011 © Bruno Chareyre <bruno.chareyre@grenoble-inp.fr>
# A dummy test, just to give an example, and detect possible path problems
from __future__ import print_function
from yade import pack,export,plot
import math,os,sys
print('checkTest mechanism')

#Typical structure of a checkTest:

#do something and get a result...

if 0: #put a condition on the result here, is it the expected result? else:
	raise YadeCheckError("Dummy failed (we know it will not happen here, you get the idea).")
