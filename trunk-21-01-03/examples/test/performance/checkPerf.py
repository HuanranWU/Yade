# -*- coding: utf-8

from __future__ import print_function

from builtins import range
from yade import pack,export,geom,timing,bodiesHandling
import time,numpy

# The original regular test parameters
radRAD=[
	23.658,				#5000 elements
	40.455,				#25000 elements
	50.97,				#50000 elements
	64.218,				#100000 elements
	80.91,				#200000 elements
	#109.811,			#500000 elements
]

iterN=[
	12000,			#5000 elements
	2500,				#25000 elements
	1400,				#50000 elements
	800,				#100000 elements
	200,				#200000 elements
	#10,				#500000 elements
]

coefCor=[
	110,
	28,
	18,
	9,
	2,
	#0.1,
]

numberTests = 3
numThreads=os.environ['OMP_NUM_THREADS'] if (len(os.environ['OMP_NUM_THREADS'])==2) else ('0'+os.environ['OMP_NUM_THREADS'])

if(yade.runtime.opts.stdperformance):
	radRAD      = [30.77]
	iterN       = [7000]
	coefCor     = [9]
	numberTests = 10
	print("\033[93m Running --stdperformance test: 10000 spheres, "+str(iterN)+" iterations, average over "+str(numberTests)+" runs. Threads: "+str(numThreads)+"\033[0m")
elif(yade.runtime.opts.quickperformance):
	radRAD      = [23.658]
	iterN       = [2000]
	coefCor     = [110]
	numberTests = 2
	print("\033[93m Running --quickperformance test: 5000 spheres, "+str(iterN)+" iterations, average over "+str(numberTests)+" runs. Threads: "+str(numThreads)+"\033[0m")
else:
	print("\033[93m Running regular --performance test. Threads: "+str(numThreads)+"\033[0m")

iterVel=[]
testTime=[]
particlesNumber=[]
data=[]

initIter = 1	# number of initial iterations excluded from performance check (e.g. Collider is initialised in first step therefore exclude first step from performance check)

tStartAll=time.time()

def calcAverageSoFar(numberSoFar,iterVelSoFar,lenTests,ii):
	iterVelNumpy=numpy.empty(numberSoFar)
	for z in range(numberSoFar):
		iterVelNumpy[z]=iterVelSoFar[z*lenTests+ii]
	avgVel = numpy.average(iterVelNumpy)
	dispVel = numpy.std(iterVelNumpy)/numpy.average(iterVelNumpy)*100.0
	return iterVelNumpy , avgVel , dispVel


while len(iterVel) < (numberTests*len(radRAD)):
	for i in range(len(radRAD)):
		rR = radRAD[i]
		nbIter=iterN[i]
		O.reset()
		
		tc=0.001
		en=.003
		es=.003
		frictionAngle=radians(35)
		density=2300
		defMat=O.materials.append(ViscElMat(density=density,frictionAngle=frictionAngle,tc=tc,en=en,et=es))
		
		O.dt=.1*tc # time step
		rad=0.5 # particle radius
		tolerance = 0.0001
		
		SpheresID=[]
		SpheresID+=O.bodies.append(pack.regularHexa(pack.inSphere((Vector3(0.0,0.0,0.0)),rad),radius=rad/rR,gap=rad/rR*0.5,material=defMat))
		
		geometryParameters = bodiesHandling.spheresPackDimensions(SpheresID)
		print(len(SpheresID))
		
		floorId=[]
		floorId+=O.bodies.append(geom.facetBox(geometryParameters['center'],geometryParameters['extends']/2.0*1.05,material=defMat)) #Floor
		
		#Calculate the mass of spheres
		sphMass = getSpheresVolume()*density
		
		# Create engines
		O.engines=[
			ForceResetter(),
			InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb()]),
			InteractionLoop(
				[Ig2_Sphere_Sphere_ScGeom(), Ig2_Facet_Sphere_ScGeom()],
				[Ip2_ViscElMat_ViscElMat_ViscElPhys()],
				[Law2_ScGeom_ViscElPhys_Basic()],
			),
			NewtonIntegrator(damping=0,gravity=[0,-9.81,0]),
		]
		
		print("number of bodies %d"%len(O.bodies))
		# Initialize the collider else it is not possible to compare the results with different nbIter
		O.run(initIter,1)
		O.timingEnabled=True
		timing.reset()
		tStart=time.time()
		# run nbIter iterations
		O.run(nbIter)
		O.wait()
		
		tEnd=time.time()
		print()
		print('Elapsed ', tEnd-tStart, ' sec')
		print('Performance ', nbIter/(tEnd-tStart), ' iter/sec')
		print('Extrapolation on 1e5 iters ', (tEnd-tStart)/nbIter*1e5/3600., ' hours')
		print("=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*")
		timing.stats()
		iterVel += [nbIter/(tEnd-tStart)]
		testTime += [tEnd-tStart]
		particlesNumber += [len(O.bodies)]
	# in --stdperformance test an attempt is made to find an average with low standard deviation.
	if((len(radRAD) == 1) and yade.runtime.opts.stdperformance and ( numberTests >= 7 ) and ( len(iterVel) >= numberTests )):
		# this loop will keep removing outliers which are too far away from average result. Must have have at least 7 results for this to work
		# If standard deviation is too big, then remove largest outliers. Better to check three times, otherwise if the situation changed
		# (another program was stopped) only the last result would be kept being removed.
		for outliers in range(3):
			iterVelNumpy , avgVel , dispVel = calcAverageSoFar( len(iterVel) , iterVel , len(radRAD) , 0 )
			# remove only if standard deviation is too big.
			if (dispVel>1.0):
				pos=None
				maxDiff=0
				for z in range(len(iterVel)):
					diff = abs(avgVel-iterVel[z])
					if (diff > maxDiff):
						maxDiff = diff
						pos     = z
					#print("\033[93m maxDiff="+str(maxDiff)+" pos="+str(pos)+"\033[0m")
				# will remove position 'pos' from the results.
				print("\033[93m Standard deviation "+str(dispVel)+" too large, removing result number "+str(pos)+"\033[0m, current average vel: "+str(avgVel))
				del iterVel[pos]
				del testTime[pos]
				del particlesNumber[pos]

tEndAll=time.time()
commonTime = tEndAll-tStartAll

print("Common time ", commonTime, "s")
print()
print()

print("___________________________________________________")
print()
print("\033[93m SUMMARY\033[0m")
print()
scoreIterVel=0.0

for i in range(len(radRAD)):
	iterVelNumpy , avgVel , dispVel = calcAverageSoFar( numberTests , iterVel , len(radRAD) , i )
	if (dispVel>10.):
		print("Calculation velocity is unstable, try to close all programs and start performance tests again")
	
	print(particlesNumber[i]," spheres,\033[93m calculation velocity=",avgVel, "iter/sec +/-",dispVel,"%\033[0m")
	data+=[[particlesNumber[i],avgVel,avgVel*dispVel/100.]]
	scoreIterVel+=avgVel/coefCor[i]*1000.0
print()
scoreIterVel = int(scoreIterVel)
## write output file for graph
import subprocess
cmd = "cat /proc/cpuinfo | grep \'model name\' | uniq"
#processor = subprocess.check_output(cmd, shell=True).lstrip('model name\t:').strip() # needs python >=2.7.0
process = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
processor = process.communicate()[0].decode().lstrip('model name\t:').strip()
cmd = "cat /proc/cpuinfo | grep processor | wc -l"
#cores = subprocess.check_output("cat /proc/cpuinfo | grep processor | wc -l", shell=True).strip() # needs python >=2.7.0 
process = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
cores = process.communicate()[0].decode().strip()
header='# '+ processor + ' ('+cores+' cores)'
filename=version+"_j"+numThreads+".dat"
#numpy.savetxt(filename,numpy.array(data),fmt=['%i','%g','%g'], header=header) # needs numpy >=1.7.0
fid = open( filename, 'w' )
fid.write(header+"\n")
for l in data:
	fid.write("%d %f %f\n"%(l[0],l[1],l[2]))
fid.close()
print("Summary saved to "+filename)
print()
print("SCORE: " + str(scoreIterVel))
print("Number of threads: ", os.environ['OMP_NUM_THREADS'])
print("___________________________________________________")
print()

if(not yade.runtime.opts.quickperformance):
	print("CPU info:")
	cmd = "lscpu"
	#cpuinfo=subprocess.check_output(cmd, shell=True) # needs python >=2.7.0
	process = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
	cpuinfo = process.communicate()[0].decode().lstrip('model name\t:').strip()
	print(cpuinfo)

sys.exit(0)
