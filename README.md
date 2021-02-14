# CMPUT379-Assign3

How to run (Linux Ubuntu):
  1. Find the IPv4 address of the device.
  2. In a terminal, enter the directory of this repository. 
  3. Enter "make".
  4. type "./server port_number" where port_number is a number between 5000 and 64000 (inclusive).
  5. Open another terminal and enter "./ client port_number ipv4_address <input.txt" where port_number is the same as the one entered for server, ipv4_address is the IPv4 address of the device, and input.txt is a text file with the transaction items. There is a 60 second inactivity timeout period for the server.
  6. If successful, the server will output each transaction to the terminal, and on termination after timeout will display a summary.
  7. To remove all log files created and executables, enter "make clean".

I recieved 100/100 on this assignment as seen in the PDF in this repository.

![](Assignment3Mark.png)
