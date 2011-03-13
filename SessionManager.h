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

rsdef(self, New);
def(void, Destroy);
def(void, SetBackend, BackendSessionInterface *backend);
def(SessionInstance, CreateSession);
def(void, DestroySession, SessionInstance sess);
def(RdString, Register, SessionInstance instance);
def(SessionInstance, Resolve, RdString id);
def(void, Unlink, RdString id);

#undef self
