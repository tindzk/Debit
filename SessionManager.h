#import <String.h>
#import <Integer.h>

#import "Session.h"

#define self SessionManager

Interface(BackendSession) {
	size_t size;

	void (*init)   (Instance $this);
	void (*destroy)(Instance $this);
};

record(SessionItem) {
	String id;
	Session *sess;
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
def(Session *, CreateSession);
def(void, DestroySession, Session *sess);
def(RdString, Register, Session *sess);
def(Session *, Resolve, RdString id);
def(void, Unlink, RdString id);

#undef self
