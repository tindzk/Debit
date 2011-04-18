#import <Connection.h>
#import <SocketSession.h>
#import <SocketConnection.h>

#import "RequestPacket.h"

#define self ResponseSender

class {
	bool           persistent;
	Logger         *logger;
	SocketSession  session;
	RequestPackets packets;
};

def(void, Init, Connection_Client *client, Logger *logger);
def(void, Destroy);
def(RequestPacket *, GetPacket);
def(FrontController *, GetController);
def(void, NewPacket);
def(void, SendResponse, RequestPacket *packet);
def(void, Continue);
def(bool, Close);

#undef self
