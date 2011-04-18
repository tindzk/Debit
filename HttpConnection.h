#import <HTTP.h>
#import <String.h>
#import <Logger.h>
#import <Integer.h>
#import <Connection.h>
#import <LinkedList.h>
#import <HTTP/Server.h>
#import <HTTP/Status.h>
#import <HTTP/Envelope.h>
#import <SocketConnection.h>

#import "Router.h"
#import "Request.h"
#import "ResponseSender.h"
#import "BufferResponse.h"
#import "FrontController.h"
#import "ResourceInterface.h"

#define self HttpConnection

ExportAnonImpl(self, Connection);

#undef self
