# Webcam Project Rev 2

This project updates my prior proj_cam by replacing the network code,
previously implemented using UDP Hole Punching, with TCP/IP. My router's port forwarding
capability will now be used to direct external TCP traffic to the Raspberry Pi
computers that run the Webcam Servers.

# History

The UDP Hole Punching implementation required a server program, which I've
had running on a Google Cloud VM. This server program was used to establish
the UDP Hole Punching connection between the Viewer program and the webcam 
devices. The webcam devices are on my NAT router network (19.168.1.x), and the
Viewer program can be run from anywhere.

UDP Hole Punching was interesting to develop and a good learning experience.
However, keeping the Google Cloud VM running for the past 6 years is an expense 
I no longer want to incur ($27/month). 

My 4 Raspberry Pi computers, running the Webcam Server program, have been running
for the past 6 years. I've had to replace a couple of failed SD Cards and power adapters. 
The 4 Raspberry Pi computers have not failed.

