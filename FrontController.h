#import <String.h>
#import <Logger.h>

#import "Response.h"
#import "SessionManager.h"
#import "ResourceInterface.h"

#undef self
#define self FrontController

class {
	Request           request;
	GenericInstance   instance;
	ResourceRoute     *route;
	ResourceInterface *resource;
};

def(void, Init);
def(void, Destroy);
def(bool, HasResource);
def(void, Reset);
def(void, SetCookie, String name, String value);
def(void, SetHeader, String name, String value);
def(String *, GetMemberAddr, String name);
def(bool, Store, String name, String value);
def(void, SetMethod, HTTP_Method method);
def(void, SetRoute, ResourceRoute *route);
def(void, SetResource, ResourceInterface *resource);
def(void, CreateResource);
def(void, HandleRequest, ResponseInstance resp);
