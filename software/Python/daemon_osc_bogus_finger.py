"""
Author: Yuri De Pra
Date: 2019-12-09

Basic OSC server for Bogus Finger control

This program:
- listens to the given address/port (default 127.0.0.1:5005) for OSC commands
- open the serial connection with the Bogus Finger
- maps OSC commands to the serial channel

"""

import argparse
import serial
import time
import threading
from pythonosc import dispatcher
from pythonosc import osc_server


#parameters
com_port= "COM32"
baud_rate= "2000000"

#global vairables
commandToCheck = False #set as True when waiting for device acknoledgment
lastAck = ""
motor_moving = False
forceHolding = False
disableOnBoardCmd = False
abort= False

# thread to receive serial communication asynch
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

#send command to serial channel			
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

#OSC command handlers
def setForce_handler(unused_addr, args, force):
	try:
		print("[{0}] ~ {1}".format(args[0], force))
		#set the force
		myForce = 'f'+str(force*100)
		sendCommand(myForce)
	
	except ValueError as e:
		print("setForce exception ", e)
	
def moveForce_handler(unused_addr, args,_):
	try:
		sendCommand('mf')
		print('Start moving')
	except Exception as e:
		print("move Force exception ", e)
	
def stop_handler(unused_addr, args,_):
	try:
		sendCommand('stop')
		print('Stop')
	except Exception as e:
		print("command stop exception ", e)
	
def home_handler(unused_addr, args,_):
	try:
		sendCommand('home')
		print('Home')
	except Exception as e:
		print("command home exception ", e)
  
def zero_handler(unused_addr, args,_):
	try:
		sendCommand('zero')
		print('Current position set as new Zero')
	except Exception as e:
		print("command set zero exception ", e)
  
if __name__ == "__main__":

	#start serial thread
	ser = serial.Serial(com_port,baud_rate, timeout=1)
	ser.flushInput()
	ser.flushOutput()
	t = threading.Thread(target=read_from_port, args=(ser,))
	t.start()
	
	#wait the communication open and check it
	time.sleep(3)
	sendCommand('check')
	print("COM Connection ready")
	disableOnBoardCmd = True # only remote commands now
	
	#parse script arguments
	parser = argparse.ArgumentParser()
	parser.add_argument("--ip", default="127.0.0.1", help="The ip to listen on")
	parser.add_argument("--port", type=int, default=5005, help="The port to listen on")
	args = parser.parse_args()

	#map OSC funtion to device Command handlers
	dispatcher = dispatcher.Dispatcher()
	dispatcher.map("/bFinger/setForce", setForce_handler, "Set Force")
	dispatcher.map("/bFinger/moveForce", moveForce_handler, "Move Force")
	dispatcher.map("/bFinger/stop", stop_handler, "Stop command")
	dispatcher.map("/bFinger/home", home_handler, "Home command")
	dispatcher.map("/bFinger/zero", zero_handler, "Set zero command")
	
	#start OSC server with the given parameters
	server = osc_server.ThreadingOSCUDPServer((args.ip, args.port), dispatcher)
	print("OSC server active on {}".format(server.server_address))
	server.serve_forever()
	

	
	
	
	