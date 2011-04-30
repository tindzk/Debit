#import "SessionManager.h"

#define self SessionManager

/* TODO Use a more efficient data structure. */

Singleton(self);
SingletonDestructor(self);

rsdef(self, New) {
	return (self) {
		.sessions = Sessions_New(1024),
		.backend  = NULL
	};
}

static def(void, DestroyItem, SessionItem *item) {
	String_Destroy(&item->id);
	call(DestroySession, item->sess);
}

def(void, Destroy) {
	each(sess, this->sessions) {
		call(DestroyItem, sess);
	}

	Sessions_Free(this->sessions);
}

def(void, SetBackend, BackendSessionInterface *backend) {
	this->backend = backend;
}

def(Session *, CreateSession) {
	if (this->backend != NULL) {
		Session *sess = Session_New(this->backend->size);
		this->backend->init(Session_GetData(sess));

		return sess;
	}

	return Session_New(0);
}

def(void, DestroySession, Session *sess) {
	if (this->backend != NULL) {
		this->backend->destroy(Session_GetData(sess));
	}

	Session_Destroy(sess);
}

/* TODO Use a better algorithm. */
def(String, GetUniqueId) {
	Time_UnixEpoch time = Time_GetCurrent();
	return Integer_ToString((u32) time.sec);
}

def(RdString, Register, Session *sess) {
	sess->ref = true;

	SessionItem item = {
		.id   = call(GetUniqueId),
		.sess = sess
	};

	each(sess, this->sessions) {
		if (sess == NULL) {
			*sess = item;
			goto out;
		}
	}

	Sessions_Push(&this->sessions, item);

out:
	return item.id.rd;
}

def(Session *, Resolve, RdString id) {
	each(sess, this->sessions) {
		if (String_Equals(sess->id.rd, id)) {
			return sess->sess;
		}
	}

	return NULL;
}

def(void, Unlink, RdString id) {
	each(sess, this->sessions) {
		if (String_Equals(sess->id.rd, id)) {
			call(DestroyItem, sess);
			sess->sess = NULL;

			break;
		}
	}
}
