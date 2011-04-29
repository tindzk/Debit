#import <Server.h>
#import <SocketSession.h>
#import <SocketConnection.h>

#import "RequestPacket.h"

#define self ResponseSender

class {
	Logger         *logger;
	Server_Client  *client;
	SocketSession  session;
	RequestPackets packets;
};

rsdef(self, New, Server_Client *client, Logger *logger);
def(void, DropPacketsUntil, RequestPacket *except);
def(void, Destroy);
def(RequestPacket *, GetPacket);
def(FrontController *, GetController);
def(void, NewPacket);
def(void, Flush);
def(void, Continue);

#undef self
