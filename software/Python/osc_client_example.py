"""
Author: Yuri De Pra
Date: 2019-12-09

Simple example OSC client for bogus finger

This program:
   - Open the connection with the server on the given port/address (default 127.0.0.1:5005)
   - Set current position as zero/home
   - Send a series of force to apply
   - Return to home position
"""

import argparse
import random
import time
from pythonosc import udp_client


if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("--ip", default="127.0.0.1", help="The ip of the OSC server")
	parser.add_argument("--port", type=int, default=5005, help="The port the OSC server is listening on")
	args = parser.parse_args()
	#open connection with server
	client = udp_client.SimpleUDPClient(args.ip, args.port)
	time.sleep(2)
	#set current position as zero/home
	client.send_message("/bFinger/zero", 0)
	time.sleep(3)
	#applies different forces (from 1N to 4N)
	for force in range(1,5):
		client.send_message("/bFinger/setForce", force)
		time.sleep(2)
		client.send_message("/bFinger/moveForce", 0)
		time.sleep(6) #wait 3 minutes to reach the desired point and stabilize
		client.send_message("/bFinger/stop", 0)
		#put here your code...
		time.sleep(1)
	
	
	client.send_message("/bFinger/home", 0)