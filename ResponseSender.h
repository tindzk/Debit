#import <Server.h>
#import <SocketSession.h>
#import <SocketConnection.h>

#import "RequestPacket.h"

#define self ResponseSender

class {
	bool           complete;
	bool           persistent;
	Logger         *logger;
	Server_Client  *client;
	SocketSession  session;
	RequestPackets packets;
};

def(void, Init, Server_Client *client, Logger *logger);
def(void, Destroy);
def(RequestPacket *, GetPacket);
def(FrontController *, GetController);
def(void, NewPacket);
def(void, SendResponse, RequestPacket *packet);
def(void, Continue);
def(bool, Close);

static inline def(void, SetComplete, bool complete) {
	this->complete = complete;
}

#undef self
