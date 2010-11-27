#import <String.h>

record(MimeType) {
	String extension;
	String mimeType;
};

extern MimeType MimeTypes[];
extern size_t MimeTypes_Length;
