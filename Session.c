#import "Session.h"

#define self Session

def(void, Init) {
	this->ref = false;
	this->userId = -1;
	this->changed = false;
	this->lastActivity = (Time_UnixEpoch) { 0, 0 };
	this->data = HeapString(0);
}

def(void, Destroy) {
	String_Destroy(&this->data);
}

def(void, Reset) {
	String_Destroy(&this->data);

	call(Init);
}

def(bool, IsGuest) {
	return this->userId == -1;
}

def(bool, IsUser) {
	return this->userId != -1;
}

def(bool, IsReferenced) {
	return this->ref;
}

def(bool, IsExpired) {
	if (this->lastActivity.sec == 0) {
		/* Just created/reset. */
		return false;
	}

	Time_UnixEpoch cur = Time_GetCurrent();
	return (cur.sec - this->lastActivity.sec) > 60 * 60 * 24; /* One day. */
}

def(void, SetUserId, size_t id) {
	this->userId = id;
	this->changed = true;
}

def(String, GetData) {
	return String_Disown(this->data);
}

def(void, SetData, String data) {
	String_Copy(&this->data, data);
	this->changed = true;
}

def(void, Logout) {
	this->userId = -1;
	this->changed = true;
}

def(void, Touch) {
	this->lastActivity = Time_GetCurrent();
	this->changed = true;
}

def(bool, HasChanged) {
	return this->changed;
}
