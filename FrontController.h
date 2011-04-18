#import <String.h>
#import <Logger.h>

#import "Session.h"
#import "Request.h"
#import "Response.h"
#import "ResourceInterface.h"

#define self FrontController

class {
	GenericInstance   instance;
	Logger            *logger;
	ResourceRoute     *route;
	ResourceInterface *resource;
};

rsdef(self, New, Logger *logger);
def(void, Destroy);
def(bool, HasResource);
def(String *, GetMemberAddr, RdString name);
def(bool, Store, RdString name, RdString value);
def(void, StoreEx, RdString name, RdString value);
def(void, CreateResource, ResourceRoute *route, ResourceInterface *resource);
def(void, Dispatch, Session *sess, Request *request, Response *response);
def(void, PostDispatch, Session *sess, Request *request, Response *response);
def(void, Error, HTTP_Status status, RdString msg, Response *response);

#undef self
