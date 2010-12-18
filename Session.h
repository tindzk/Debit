#import <Time.h>
#import <Date.h>

#define self Session

class {
	bool ref;
	bool changed;

	Time_UnixEpoch lastActivity;

	char data[];
};

sdef(SessionInstance, New, size_t data);
def(void, Init);
def(void, Destroy);
def(void, Reset);
def(GenericInstance, GetData);
def(bool, IsReferenced);
def(bool, HasChanged);
def(void, SetChanged);
def(bool, IsExpired);
def(void, Touch);

#undef self
