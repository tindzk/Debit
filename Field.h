#import <Array.h>
#import <String.h>
#import <Integer.h>

record(FormError) {
	String field;
	String msg;
};

Array(FormError, FormErrors);

String Field_GetValue(String val, String field, FormErrors **err);

s8  Field_GetInt8(String val, String field, FormErrors **err);
u8 Field_GetUInt8(String val, String field, FormErrors **err);

s32  Field_GetInt32(String val, String field, FormErrors **err);
u32 Field_GetUInt32(String val, String field, FormErrors **err);

bool Field_IsSurroundedBySpaces(String s);
bool Field_IsValidEmail(String s);
