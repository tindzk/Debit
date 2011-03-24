#import "Session.h"

#define self Session

sdef(self *, New, size_t data) {
	self *obj = Generic_New(sizeof(self) + data).object;
	scall(Reset, obj);
	return obj;
}

def(void, Free) {
	Generic_Free((SessionInstance) this);
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
