#import "Session.h"

def(void, Init) {
	this->ref = false;
	this->userId = -1;
	this->changed = false;
	this->lastActivity = (Time_UnixEpoch) { 0, 0 };
}

def(void, Destroy) {

}

def(void, Reset) {
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

	Time_UnixEpoch cur = Time_GetCurrentUnixTime();
	return (cur.sec - this->lastActivity.sec) > 60 * 60 * 24; /* One day. */
}

def(void, SetUserId, size_t id) {
	this->userId = id;
	this->changed = true;
}

def(void, Logout) {
	this->userId = -1;
	this->changed = true;
}

def(void, Touch) {
	this->lastActivity = Time_GetCurrentUnixTime();
	this->changed = true;
}

def(bool, HasChanged) {
	return this->changed;
}
