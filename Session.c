#import "Session.h"

#define self Session

sdef(self *, New, size_t data) {
	DynObject obj = DynObject_New(sizeof(self) + data);
	scall(Reset, obj.inst);
	return obj.addr;
}

def(void, Destroy) {
	DynObject_Destroy(this);
}

def(void, Reset) {
	this->ref = false;
	this->changed = false;
	this->lastActivity = (Time_UnixEpoch) { 0, 0 };
}

def(void *, GetData) {
	return (void *) this->data;
}

def(bool, IsExpired) {
	if (this->lastActivity.sec == 0) {
		/* Just created/reset. */
		return false;
	}

	Time_UnixEpoch cur = Time_GetCurrent();
	return (cur.sec - this->lastActivity.sec) > Date_SecondsDay;
}

def(bool, IsReferenced) {
	return this->ref;
}

def(bool, HasChanged) {
	return this->changed;
}

def(void, SetChanged) {
	this->changed = true;
}

def(void, Touch) {
	this->lastActivity = Time_GetCurrent();
	this->changed = true;
}
