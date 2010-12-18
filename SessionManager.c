#import "SessionManager.h"

#define self SessionManager

/* TODO Use a more efficient data structure. */

Singleton(self);
SingletonDestructor(self);

def(void, Init) {
	this->sessions = Sessions_New(1024);
	this->backend  = NULL;
}

static def(void, DestroyItem, SessionItem *item) {
	String_Destroy(&item->id);
	call(DestroySession, item->instance);
}

def(void, Destroy) {
	foreach (sess, this->sessions) {
		call(DestroyItem, sess);
	}

	Sessions_Free(this->sessions);
}

def(void, SetBackend, BackendSessionInterface *backend) {
	this->backend = backend;
}

def(SessionInstance, CreateSession) {
	SessionInstance sess =
		Session_New(
			(this->backend != NULL)
				? this->backend->size
				: 0);

	Session_Init(sess);

	if (this->backend != NULL) {
		this->backend->init(Session_GetData(sess));
	}

	return sess;
}

def(void, DestroySession, SessionInstance sess) {
	if (this->backend != NULL) {
		this->backend->destroy(Session_GetData(sess));
	}

	Session_Destroy(sess);

	Generic_Free(sess);
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
			call(DestroyItem, sess);
			sess->instance = Session_Null();

			break;
		}
	}
}
