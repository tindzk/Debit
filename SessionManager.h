#import <String.h>
#import <Integer.h>

#import "Session.h"

#define self SessionManager

Interface(BackendSession) {
	size_t size;

	Method(void, init);
	Method(void, destroy);
};

record(SessionItem) {
	String id;
	SessionInstance instance;
};

Array(SessionItem, Sessions);

class {
	Sessions *sessions;
	BackendSessionInterface *backend;
};

SingletonPrototype(self);

def(void, Init);
def(void, Destroy);
def(void, SetBackend, BackendSessionInterface *backend);
def(SessionInstance, CreateSession);
def(void, DestroySession, SessionInstance sess);
def(String, Register, SessionInstance instance);
def(SessionInstance, Resolve, String id);
def(void, Unlink, String id);

#undef self
