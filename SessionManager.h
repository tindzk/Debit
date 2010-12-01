#import <String.h>
#import <Integer.h>

#import "Session.h"

#define self SessionManager

record(SessionItem) {
	String id;
	SessionInstance instance;
};

Array_Define(SessionItem, Sessions);

class {
	Sessions *sessions;
};

SingletonPrototype(self);

def(void, Init);
def(void, Destroy);
def(String, Register, SessionInstance instance);
def(SessionInstance, Resolve, String id);
def(void, Unlink, String id);

#undef self
