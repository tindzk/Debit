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
	RdStringArray *pathElems;
	RdStringArray *routeElems;

	ResourceRoute *route;

	ResourceInterface *resource;
};

DefineCallback(ref(OnPart), void, RdString name, RdString value);

SingletonPrototype(self);

rsdef(self, New);
def(void, Destroy);
def(void, DestroyMatch, MatchingRoute match);
def(void, AddResource, ResourceInterface *resource);
def(bool, IsRouteMatching, RdStringArray *route, RdStringArray *path);
def(void, ExtractParts, RdStringArray *route, RdStringArray *path, ref(OnPart) onPart);
def(MatchingRoute, FindRoute, RdString path);
def(MatchingRoute, GetDefaultRoute);

#undef self
