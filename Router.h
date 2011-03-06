#import <Array.h>
#import <String.h>

#import "ResourceInterface.h"

#define self Router

// @exc NestedBrackets

Array(ResourceInterface *, Resources);

class {
	Resources *resources;
};

record(MatchingRoute) {
	ProtStringArray *pathElems;
	ProtStringArray *routeElems;

	ResourceRoute *route;

	ResourceInterface *resource;
};

DefineCallback(ref(OnPart), void, ProtString name, ProtString value);

SingletonPrototype(self);

rsdef(self, New);
def(void, Destroy);
def(void, DestroyMatch, MatchingRoute match);
def(void, AddResource, ResourceInterface *resource);
def(bool, IsRouteMatching, ProtStringArray *route, ProtStringArray *path);
def(void, ExtractParts, ProtStringArray *route, ProtStringArray *path, ref(OnPart) onPart);
def(MatchingRoute, FindRoute, ProtString path);
def(MatchingRoute, GetDefaultRoute);

#undef self
