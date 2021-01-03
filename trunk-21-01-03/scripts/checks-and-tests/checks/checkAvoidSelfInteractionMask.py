# encoding: utf-8
from __future__ import print_function

# This is based on https://answers.launchpad.net/yade/+question/682290 script by Jan Stránský and Bruno Chareyre
print('testing collider.avoidSelfInteractionMask')

#### This is useful for printing the linenumber in the script
# import inspect
# print inspect.currentframe().f_lineno

maskTest= [
 [ 0b00 , "bb br gg gr rr" ]
,[ 0b01 , "bb br gr"       ]
,[ 0b10 , "br gg gr"       ]
,[ 0b11 , "br gr"          ]
]
# mask values, "colors"
mb,mg,mr = 0b10, 0b01, 0b11

for mask in maskTest:
	maskValue = mask[0]
	maskRes   = mask[1]
	# avoid errors about InsertionSortCollider.verletDist>0, but unable to locate NewtonIntegrator within O.engines by setting some default engines
	O.engines=[
		ForceResetter(),
		InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb()], label="collider"),
		InteractionLoop(
			[Ig2_Sphere_Sphere_ScGeom(), Ig2_Facet_Sphere_ScGeom()],
			[Ip2_ViscElMat_ViscElMat_ViscElPhys()],
			[Law2_ScGeom_ViscElPhys_Basic()],
		),
		NewtonIntegrator(damping=0.0,gravity=[0.0,0.0,-9.81])
	]

	# testing spheres, 2 of each color
	sb1 = sphere((0,0,0),1,mask=mb)
	sb2 = sphere((0,0.1,0),1,mask=mb)
	sg1 = sphere((0.1,0,0),1,mask=mg)
	sg2 = sphere((0,0,0.1),1,mask=mg)
	sr1 = sphere((-0.1,0,0),1,mask=mr)
	sr2 = sphere((0,-0.1,0),1,mask=mr)
	O.bodies.append((sb1,sb2,sg1,sg2,sr1,sr2))

	collider.avoidSelfInteractionMask = maskValue
	collider.boundDispatcher.__call__()
	collider.__call__()

	idss = [(i.id1,i.id2) for i in O.interactions.all()] # id couples for each interaction
	bss = [[O.bodies[i] for i in ids] for ids in idss]   # body couple of each interaction
	mss = [[b.mask for b in bs] for bs in bss]           # mask couple of each interaction
	mss = [tuple(sorted(ms)) for ms in mss]              # make the mask couple of type tuple (needs by set below). Also sort it for proper deletion of duplicates
	mss = set(mss)                                       # make the entries unique (i.e. deleting duplicates)
	m2c = {mb:"b", mg:"g", mr:"r"}
	#css = ["".join({2: 'b', 1: 'g', 3: 'r'}[m] for m in ms) for ms in mss] # convert mask couples to strings
	css = ["".join(m2c[m] for m in ms) for ms in mss]    # convert mask couples to strings
	css.sort()                                           # sort the result
	css = " ".join(css)                                  # make it one string
	print(maskValue,css) # print interacting color couples

	if(css != maskRes):
		collider.avoidSelfInteractionMask=0
		raise YadeCheckError("collider.avoidSelfInteractionMask failed for "+str(maskValue)+" should be: "+str(maskRes)+", but the result is: "+str(css))

	O.reset()

# FIXME: O.reset does not reset value of collider.avoidSelfInteractionMask, see https://gitlab.com/yade-dev/trunk/issues/106
collider.avoidSelfInteractionMask=0

