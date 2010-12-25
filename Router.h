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
	StringArray *pathElems;
	StringArray *routeElems;

	ResourceRoute *route;

	ResourceInterface *resource;
};

DefineCallback(ref(OnPart), void, String name, String value);

SingletonPrototype(self);

def(void, Init);
def(void, Destroy);
def(void, DestroyMatch, MatchingRoute match);
def(void, AddResource, ResourceInterface *resource);
def(bool, IsRouteMatching, StringArray *route, StringArray *path);
def(void, ExtractParts, StringArray *route, StringArray *path, ref(OnPart) onPart);
def(MatchingRoute, FindRoute, String path);
def(MatchingRoute, GetDefaultRoute);

#undef self
