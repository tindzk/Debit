#import <Time.h>

#undef self
#define self Session

class(self) {
	bool ref;
	bool changed;
	ssize_t userId;
	Time_UnixEpoch lastActivity;
};

ExtendClass(self);

def(void, Init);
def(void, Destroy);
def(void, Reset);
def(bool, IsGuest);
def(bool, IsUser);
def(bool, IsReferenced);
def(bool, IsExpired);
def(void, SetUserId, size_t id);
def(void, Logout);
def(void, Touch);
def(bool, HasChanged);
