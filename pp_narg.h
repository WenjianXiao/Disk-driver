#ifndef MICRO_PP_NARG_H
#define MICRO_PP_NARG_H
/* The PP_NARG macro returns the number of arguments that have been
 * passed to it. limit = 64
 */

#define PP_NARG(...) PP_NARG_(0, ##__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(_0,\
		          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
		         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
		         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
		         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
		         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
		         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
		         _61,_62,_63,N,...) N
#define PP_RSEQ_N() \
	         63,62,61,60, \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

/* The PP_SHORT_NARG macro returns the number of arguments that have been
 * passed to it. limit = 16
 */
#define PP_SHORT_NARG(...) PP_SHORT_NARG_(0, ##__VA_ARGS__,PP_SHORT_RSEQ_N())
#define PP_SHORT_NARG_(...) PP_SHORT_ARG_N(__VA_ARGS__)
#define PP_SHORT_ARG_N(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
		         _11,_12,_13,_14,_15,_16, N,...) N
#define PP_SHORT_RSEQ_N() 16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0

#define FOR_EACH_0(...)
#define FOR_EACH_1(what, x) what(x)
#define FOR_EACH_2(what, x, ...)\
	what(x);\
	FOR_EACH_1(what, __VA_ARGS__);
#define FOR_EACH_3(what, x, ...)\
	what(x);\
	FOR_EACH_2(what, __VA_ARGS__);
#define FOR_EACH_4(what, x, ...)\
	what(x);\
	FOR_EACH_3(what, __VA_ARGS__);
#define FOR_EACH_5(what, x, ...)\
	what(x);\
	FOR_EACH_4(what, __VA_ARGS__);
#define FOR_EACH_6(what, x, ...)\
	what(x);\
	FOR_EACH_5(what, __VA_ARGS__);
#define FOR_EACH_7(what, x, ...)\
	what(x);\
	FOR_EACH_6(what, __VA_ARGS__);
#define FOR_EACH_8(what, x, ...)\
	what(x);\
	FOR_EACH_7(what, __VA_ARGS__);
#define FOR_EACH_9(what, x, ...)\
	what(x);\
	FOR_EACH_8(what, __VA_ARGS__);
#define FOR_EACH_10(what, x, ...)\
	what(x);\
	FOR_EACH_9(what, __VA_ARGS__);
#define FOR_EACH_11(what, x, ...)\
	what(x);\
	FOR_EACH_10(what, __VA_ARGS__);
#define FOR_EACH_12(what, x, ...)\
	what(x);\
	FOR_EACH_11(what, __VA_ARGS__);
#define FOR_EACH_13(what, x, ...)\
	what(x);\
	FOR_EACH_12(what, __VA_ARGS__);
#define FOR_EACH_14(what, x, ...)\
	what(x);\
	FOR_EACH_13(what, __VA_ARGS__);
#define FOR_EACH_15(what, x, ...)\
	what(x);\
	FOR_EACH_14(what, __VA_ARGS__);
#define CONCATENATE(arg1, arg2) arg1##arg2
#define FOR_EACH_(N, what, ...) {CONCATENATE(FOR_EACH_, N)(what, __VA_ARGS__)}
#define FOR_EACH(what, ...) FOR_EACH_(PP_SHORT_NARG(__VA_ARGS__), what, __VA_ARGS__)
#endif /*MICRO_PP_NARG_H*/
