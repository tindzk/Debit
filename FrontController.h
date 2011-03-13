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
def(void, SetCookie, RdString name, RdString value);
def(void, SetHeader, RdString name, RdString value);
def(String *, GetMemberAddr, RdString name);
def(bool, Store, RdString name, RdString value);
def(void, SetMethod, HTTP_Method method);
def(void, SetRoute, ResourceRoute *route);
def(void, SetResource, ResourceInterface *resource);
def(void, CreateResource);
def(void, HandleRequest, ResponseInstance resp);

#undef self
