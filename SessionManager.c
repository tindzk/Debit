#import "SessionManager.h"

#define self SessionManager

/* TODO Use a more efficient data structure. */

Singleton(self);
SingletonDestructor(self);

def(void, Init) {
	this->sessions = Sessions_New(1024);
}

def(void, Destroy) {
	foreach (sess, this->sessions) {
		String_Destroy(&sess->id);
		Session_Destroy(sess->instance);
		Session_Free(sess->instance);
	}

	Sessions_Free(this->sessions);
}

/* TODO Use a better algorithm. */
def(String, GetUniqueId) {
	Time_UnixEpoch time = Time_GetCurrent();
	return String_Clone(Integer_ToString((u32) time.sec));
}

def(String, Register, SessionInstance instance) {
	Session *sess = Session_GetObject(instance);
	sess->ref = true;

	SessionItem item = {
		.id       = call(GetUniqueId),
		.instance = instance
	};

	foreach (sess, this->sessions) {
		if (Session_IsNull(sess->instance)) {
			*sess = item;
			goto out;
		}
	}

	Sessions_Push(&this->sessions, item);

out:
	return item.id;
}

def(SessionInstance, Resolve, String id) {
	foreach (sess, this->sessions) {
		if (String_Equals(sess->id, id)) {
			return sess->instance;
		}
	}

	return Session_Null();
}

def(void, Unlink, String id) {
	foreach (sess, this->sessions) {
		if (String_Equals(sess->id, id)) {
			Session_Destroy(sess->instance);
			Session_Free(sess->instance);

			String_Destroy(&sess->id);
			sess->instance = Session_Null();

			break;
		}
	}
}
