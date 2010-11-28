#import <HTTP.h>
#import <String.h>
#import <Logger.h>
#import <Integer.h>
#import <LinkedList.h>
#import <HTTP/Server.h>
#import <HTTP/Status.h>
#import <HTTP/Envelope.h>
#import <SocketSession.h>
#import <SocketConnection.h>

#import "Router.h"
#import "BufferResponse.h"
#import "FrontController.h"
#import "ResourceInterface.h"

#undef self
#define self HttpConnection

extern Impl(Connection);
