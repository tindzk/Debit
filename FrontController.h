#import <String.h>
#import <Logger.h>

#import "Response.h"
#import "BufferResponse.h"
#import "SessionManager.h"
#import "ResourceInterface.h"

#define self FrontController

class {
	Request           request;
	GenericInstance   instance;
	ResourceRoute     *route;
	ResourceInterface *resource;
};

rsdef(self, New);
def(void, Destroy);
def(bool, HasResource);
def(void, Reset);
def(void, SetCookie, ProtString name, ProtString value);
def(void, SetHeader, ProtString name, ProtString value);
def(String *, GetMemberAddr, ProtString name);
def(bool, Store, ProtString name, ProtString value);
def(void, SetMethod, HTTP_Method method);
def(void, SetRoute, ResourceRoute *route);
def(void, SetResource, ResourceInterface *resource);
def(void, CreateResource);
def(void, HandleRequest, ResponseInstance resp);

#undef self
