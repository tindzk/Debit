#import <String.h>
#import <Date/RFC822.h>
#import <HTTP/Method.h>
#import <HTTP/Version.h>

#define self Request

class {
	struct {
		String referer;
		String sessionId;
		HTTP_Method method;
		HTTP_Version version;
		Date_RFC822 lastModified;
	} priv;
};

static alwaysInline def(RdString, GetReferer) {
	return this->priv.referer.rd;
}

static alwaysInline def(RdString, GetSessionId) {
	return this->priv.sessionId.rd;
}

static alwaysInline def(HTTP_Method, GetMethod) {
	return this->priv.method;
}

static alwaysInline def(Date_RFC822, GetLastModified) {
	return this->priv.lastModified;
}

#undef self
