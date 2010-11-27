#import "SessionManager.h"

/* TODO Use a more efficient data structure. */

Singleton(self);
SingletonDestructor(self);

def(void, Init) {
	this->sessions = Sessions_New(1024);
}

def(void, Destroy) {
	Sessions_Free(this->sessions);
}

/* TODO Use a better algorithm. */
def(String, GetUniqueId) {
	Time_UnixEpoch time = Time_GetCurrentUnixTime();
	return String_Clone(Integer_ToString((u32) time.sec));
}

def(String, Register, SessionInstance instance) {
	Session *sess = Session_GetObject(instance);
	sess->ref = true;

	SessionItem item;

	item.id       = call(GetUniqueId);
	item.instance = instance;

	Sessions_Push(&this->sessions, item);

	return item.id;
}

def(SessionInstance, Resolve, String id) {
	forward (i, this->sessions->len) {
		if (String_Equals(this->sessions->buf[i].id, id)) {
			return this->sessions->buf[i].instance;
		}
	}

	return Session_Null();
}

def(void, Unlink, String id) {
	forward (i, this->sessions->len) {
		if (String_Equals(this->sessions->buf[i].id, id)) {
			Session *sess = Session_GetObject(this->sessions->buf[i].instance);
			sess->ref = false;

			this->sessions->buf[i].instance = Session_Null();
		}
	}
}
