#import <String.h>

record(MimeType) {
	RdString extension;
	RdString mimeType;
};

extern MimeType MimeTypes[];
extern size_t MimeTypes_Length;
