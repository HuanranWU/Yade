# 2018 © Bruno Chareyre <bruno.chareyre@grenoble-inp.fr> 

# Really a helloWorld (or almost, I couldn't refrain from adding a bit more).
# Check other examples for concrete usage of mpy in DEM simulations 

from yade import mpy as mp
mp.initialize(3)
#mp.VERBOSE_OUTPUT = True # to see more of what happens behind the scene

mp.mprint("I'm here")

if mp.rank==0:
        # say hello:
        mp.sendCommand(executors=[1,2],command="mprint('Yes I am really here')",wait=False)
        
        # get retrun values if wait=True (blocking comm., think twice)
        print( mp.sendCommand(executors="all",command="len(O.bodies)",wait=True) )
        
        ## exploit sendCommand to send data between yade instances directly with mpi4py (see mpi4py documentation)
        ## in the message we actually tell the worker to wait another message (nested comm), but the second one
        ## uses underlying mpi4py, and it handles pickable objects
        mp.sendCommand(executors=1,command="message=comm.recv(source=0); mprint('received: ',message)",wait=False)
        mp.comm.send("hello",dest=1)
        
        ## pickable objects 
        ## pay attention to pointer adresses, they are different! (as expected)
        ## this is moving data around between independent parts of memory
        mp.sendCommand(executors=1,command="O.bodies.append(Body()); O.bodies[0].shape=Sphere(radius=0.456); comm.send(O.bodies[0],dest=0); mprint('sent a ',O.bodies[0].shape)",wait=False)
        bodyFrom1 = mp.comm.recv(source=1)
        mp.mprint("received a ",bodyFrom1.shape,"with radius=",bodyFrom1.shape.radius)
  

        # More bodies
        # note that 'rank' is used instead of mp.rank, the scope of the mpy module is accessed here
        mp.sendCommand(executors=[1,2],command= "ids=O.bodies.append([sphere((xx,1.5+rank,0),0.5) for xx in range(-1,2)])",wait=True)
        
        wallId=O.bodies.append(box(center=(0,0,0),extents=(2,0,1),fixed=True)) #master only

        print( mp.sendCommand(executors="all",command="len(O.bodies)",wait=True) )
        mp.sendCommand(executors=[1,2],command= "list(map(lambda b: setattr(b,'subdomain',rank),O.bodies))", wait=True)

        print("Assigned bodies:", mp.sendCommand([1,2], "len([b for b in O.bodies if b.subdomain==rank])", True) )
        
        # Kill workers pool, then start again with a new
        # any data in the old threads is lost
        mp.disconnect()
        mp.initialize(8)
        # not all workers will be displayed because mp.MAX_RANK_OUTPUT=5 by default
        mp.sendCommand(executors="slaves",command="mprint('I am a new one')",wait=True)
        
        # no need to diconnect here, it will be called when exiting yade anyway
    

