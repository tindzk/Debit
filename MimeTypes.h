#import <String.h>

record(MimeType) {
	ProtString extension;
	ProtString mimeType;
};

extern MimeType MimeTypes[];
extern size_t MimeTypes_Length;
