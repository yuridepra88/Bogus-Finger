'''
Author: Yuri De Pra
Date: 2019-12-02

Bogus finger remote procedure
features:
- manage of serial connection with asynch receiver thread
- console log of commands and ack
- serie of force level applied with fixed time interval

example routine:
- open serial connection
- wait for zero point set on board
- for each force in forcesToApply move downward until reach the force level required
- for each force wait the correspondent amount of time and move to the next
- at the end returns to home position

'''


import serial
import time
import threading


#parameters
com_port= "COM32"
baud_rate= "2000000"

forcesToApply =  [0.5, 2, 4.9, 7.8]
timeToMaintain = [10,   10,   10,   10]


#global vairables
commandToCheck = False #set as True when waiting for device acknoledgment
lastAck = ""
motor_moving = False
forceHolding = False
disableOnBoardCmd = False
abort= False



def read_from_port(ser):
	global current_force, motor_status, lastAck, forceHolding,disableOnBoardCmd, commandToCheck, abort
	t = threading.currentThread()
	while getattr(t, "alive", True):
		try:
			serstr = ser.readline().decode()
			if(serstr != ''): 
				if (serstr.startswith('cmdAck_')):
					ackStr = serstr.replace('cmdAck_', '')
					lastAck = ackStr
					if (commandToCheck):
						commandToCheck = False
					else:
						if (disableOnBoardCmd):
							print('unexpected action')
						print('on-board action performed:')
						
				elif (serstr.startswith('Event_')):
					evntStr = serstr.replace('Event_', '')
					if (evntStr == 'Stop\r\n'):
						motor_moving = False
						
					elif (evntStr == 'Move\r\n'):
						motor_moving = True
					elif (evntStr == 'Hold\r\n'):
						forceHolding = True
				
				elif (serstr.startswith('OnBoard')):
					abort=True
				elif(serstr.startswith('Force_')):
					current_force = float(serstr.replace('Force_', ''))
				else:
					print ("not reconized: ",serstr)		
		except Exception as e: 
			print(e)	
			

			
def sendCommand(cmd):
	global ser, abort, commandToCheck
	commandToCheck = True
	strToSend = cmd+'\n'
	try:
		ser.write(strToSend.encode())
		time.sleep(1)
		while((commandToCheck) and (not abort)):
				time.sleep(0.5)
	except Exception as e: 
		print("Command send execption: "+str(e))

if __name__ == '__main__':

	#start serial thread
	ser = serial.Serial(com_port,baud_rate, timeout=1)
	ser.flushInput()
	ser.flushOutput()
	t = threading.Thread(target=read_from_port, args=(ser,))
	t.start()
	#wait the communication open and check it
	time.sleep(3)
	sendCommand('check')
	
	disableOnBoardCmd = False #on-board action required
	print('Set your Zero position on the Bogus Finger using the on-board commands')
	while (lastAck != 'New_Zero_Set'):
		time.sleep(1)
	input('Press Enter to start')
	
	#iterate the force and waitingTime arrays
	disableOnBoardCmd = True # only remote commands now
	abort= False
	for setForce, setTime in zip(forcesToApply, timeToMaintain):
	
		try:
		
			#set the force
			myForce = 'f'+str(setForce*100)
			sendCommand(myForce)
			#start mmoving
			forceHolding = False
			print('New force set at '+str(setForce)+"N")
			sendCommand('mf')
			print('Start moving')
			while ((not forceHolding) and (not abort)):
				time.sleep(0.1)
			#start wait the set time
			print('Holding for '+str(setTime)+'sec')
			startTime = time.time()
			
			while (((startTime + setTime) > time.time()) and (not abort)):
				#insert your actions here
				
				time.sleep(0.1)
			#stop to hold
			sendCommand('stop')
			if abort:
				break
		
		except Exception as e: 
			print(e)
	sendCommand('home')
		
	#stop serial thread
	t.alive= False
	t.join()
	

			