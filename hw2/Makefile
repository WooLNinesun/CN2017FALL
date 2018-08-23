# complie
all: sender.out reciver.out agent.out

sender.out: sender.c mysocket.c 
	gcc sender.c mysocket.c -o sender.out
reciver.out: receiver.c mysocket.c 
	gcc receiver.c mysocket.c -o receiver.out
agent.out: agent.c mysocket.c 
	gcc agent.c mysocket.c -o agent.out

# clear
clean:
	rm *.out
