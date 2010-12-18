#import "Session.h"

#define self Session

sdef(SessionInstance, New, size_t data) {
	Session *ptr = Memory_Alloc(sizeof(self) + data);
	return (SessionInstance) ptr;
}

def(void, Init) {
	this->ref = false;
	this->changed = false;
	this->lastActivity = (Time_UnixEpoch) { 0, 0 };
}

def(void, Destroy) {

}

def(void, Reset) {
	call(Destroy);
	call(Init);
}

def(GenericInstance, GetData) {
	return Generic_FromObject((void *) this->data);
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
