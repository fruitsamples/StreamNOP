/*	File:		StreamNOP.c	Contains:	Stream module that does nothing.	Written by:	Quinn "The Eskimo!"	Copyright:	Copyright � 1997-2000 by Apple Computer, Inc., All Rights Reserved.	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.				("Apple") in consideration of your agreement to the following terms, and your				use, installation, modification or redistribution of this Apple software				constitutes acceptance of these terms.  If you do not agree with these terms,				please do not use, install, modify or redistribute this Apple software.				In consideration of your agreement to abide by the following terms, and subject				to these terms, Apple grants you a personal, non-exclusive license, under Apple�s				copyrights in this original Apple software (the "Apple Software"), to use,				reproduce, modify and redistribute the Apple Software, with or without				modifications, in source and/or binary forms; provided that if you redistribute				the Apple Software in its entirety and without modifications, you must retain				this notice and the following text and disclaimers in all such redistributions of				the Apple Software.  Neither the name, trademarks, service marks or logos of				Apple Computer, Inc. may be used to endorse or promote products derived from the				Apple Software without specific prior written permission from Apple.  Except as				expressly stated in this notice, no other rights or licenses, express or implied,				are granted by Apple herein, including but not limited to any patent rights that				may be infringed by your derivative works or by other works in which the Apple				Software may be incorporated.				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN				COMBINATION WITH YOUR PRODUCTS.				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.	Change History (most recent first):*//////////////////////////////////////////////////////////////////////// The OT debugging macros in <OTDebug.h> require this variable to// be set.#ifndef qDebug#define qDebug	1#endif/////////////////////////////////////////////////////////////////////// Determine whether this is going to be an instrumented build or not.#ifndef INSTRUMENTATION_ACTIVE	#define INSTRUMENTATION_ACTIVE 0#else	#define INSTRUMENTATION_ACTIVE 1#endif/////////////////////////////////////////////////////////////////////// Pick up all the standard OT module stuff.#include <OpenTransportKernel.h>/////////////////////////////////////////////////////////////////////// Pick up Instrumentation SDK stuff.  We only do this if we're// actually instrumenting, so you don't even have to have the SDK// to compile the non-instumented version of the code.  If we're// not instrumenting, we compile a bunch of bogus macros that generally// compile to nothing.#if INSTRUMENTATION_ACTIVE	#include <InstrumentationMacros.h>#else	#define TRACE_SETUP		long __junk	#define LOG_ENTRY(n)	if (0) { __junk ; }	#define LOG_EXIT		if (0) { __junk ; }#endif/////////////////////////////////////////////////////////////////////// To get OTDebugStr you have to link with OpenTptMiscUtilsPPC.o, which // is not part of Universal Interfaces.  So we just define our own version, // layered on top of the lowercase debugstr.extern pascal void OTDebugStr(const char* str){	debugstr(str);}/////////////////////////////////////////////////////////////////////// Some simple routines we use in our various assertions.static Boolean IsReadQ(queue_t* q)	// Returns true if q is the read queue of a queue pair.{	return ( (q->q_flag & QREADR) != 0 );}static Boolean IsWriteQ(queue_t* q)	// Returns true if q is the write queue of a queue pair.{	return ( (q->q_flag & QREADR) == 0 );}/////////////////////////////////////////////////////////////////////// Per-Stream information// This structure is used to hold the per-stream data for the module.// While module's can use normal global variables to store real globals,// they must maintain their own per-stream data structures.  I use// mi_open_comm to allocate this data structure when the stream is// opened.  mi_open_comm stores the address of this data structure in the// read and write queue's q_ptr field, so the rest of the code// can get to it by calling the GetPerStreamData function.enum {	kStreamNOPPerStreamDataMagic = 'NOOP'};struct PerStreamData{	OSType 				magic;				// kStreamNOPPerStreamDataMagic = 'NOOP' for debugging	// Your per-stream data structures go here.};typedef struct PerStreamData PerStreamData, *PerStreamDataPtr;static PerStreamDataPtr GetPerStreamData(queue_t* readOrWriteQ)	// You can pass both the read or the write queue to this routine	// because mi_open_comm sets up both q_ptr's to point to the	// queue local data.	//	// Note that, in order to avoid the overhead of a function call,	// you would normally use inline code (or a macro)	// to get your per-stream data instead of using a separate function.	// However I think the separate function makes things clearer.	// I also acts as a central bottleneck for my debugging code.	//	// Environment: any standard STREAMS entry point{	PerStreamDataPtr streamData;		streamData = (PerStreamDataPtr) readOrWriteQ->q_ptr;	OTAssert("GetPerStreamData: what streamData", streamData != nil);	OTAssert("GetPerStreamData: Bad magic", streamData->magic == kStreamNOPPerStreamDataMagic);		return (streamData);}// mi_open_comm and mi_close_comm (and also mi_detach and mi_close_detached)// use this global to store the list of open streams to this module.static char* gStreamList = nil;/////////////////////////////////////////////////////////////////////// Open routinestatic SInt32 StreamNOPOpen(queue_t* rdq, dev_t* dev, SInt32 flag, SInt32 sflag, cred_t* creds)	// This routine is called by STREAMS when a new stream is connected to	// our module.  The bulk of the work here is done by the Mentat helper	// routine mi_open_comm.	//	// Environment: standard STREAMS entry point{	TRACE_SETUP;	int err;	PerStreamDataPtr streamData;	LOG_ENTRY( "StreamNOP:StreamNOPOpen" );		OTAssert("StreamNOPOpen: Not the read queue", IsReadQ(rdq) );	OTDebugBreak("StreamNOPOpen");		err = noErr;		// If we already have per-stream data for this stream, the stream is being reopened.	// In that case, we can just return.	// Note that we can't call GetPerStreamData because it checks that streamData is not nil.		if ( rdq->q_ptr != nil ) {		goto done;	}	// Make sure we're being opened properly -- because we're a module we	// require a "module" open.  Other possibilities are the value 0 (used	// to open a specific minor device number (ie stream) on a device driver),	// and CLONEOPEN (used to open a new stream to a device number where you	// don't care what device number you get -- the typical behaviour for	// networking (as opposed to serial) devices).		if ( (err == noErr) && (sflag != MODOPEN) ) {		err = ENXIO;	}		// Use the mi_open_comm routine to allocate our per-stream data.  Then	// zero out the entire per-stream data record and fill out the fields	// we're going to need.		if (err == noErr) {		err = mi_open_comm(&gStreamList, sizeof(PerStreamData), rdq, dev, flag, sflag, creds);		if ( err == noErr ) {			// Note that we can't call GetPerStreamData because the magic is not set up yet.			streamData = (PerStreamDataPtr) rdq->q_ptr;						OTMemzero(streamData, sizeof(PerStreamData));						streamData->magic = kStreamNOPPerStreamDataMagic;		}	}done:	LOG_EXIT;	return (err);}/////////////////////////////////////////////////////////////////////// Close routinestatic SInt32 StreamNOPClose(queue_t* rdq, SInt32 flags, cred_t* credP)	// This routine is called by STREAMS when a stream is being	// disconnected from our module (ie closed).  The bulk of the work	// is done by the magic Mentat helper routine mi_close_comm.	//	// Environment: standard STREAMS entry point{	TRACE_SETUP;	#pragma unused(flags)	#pragma unused(credP)	LOG_ENTRY( "StreamNOP:StreamNOPClose" );	OTAssert("StreamNOPClose: Not the read queue", IsReadQ(rdq) );	(void) mi_close_comm(&gStreamList, rdq);	LOG_EXIT;	return (0);}/////////////////////////////////////////////////////////////////////enum {	kNoPrimitive = -1};static long GetPrimitive(mblk_t* mp)	// GetPrimitive gets the TPI/DLPI primitive out of a message block.	// It returns kNoPrimitive if the message block is of the wrong	// type or there is no primitive.	//	// Environment: any standard STREAMS entry point{	if ((mp->b_datap->db_type == M_PROTO || mp->b_datap->db_type == M_PCPROTO) && MBLK_SIZE(mp) >= sizeof(long) ) {		return ( ( (union T_primitives*) mp->b_rptr)->type );	} else {		return ( kNoPrimitive );	}}/////////////////////////////////////////////////////////////////////// Write-side put routinestatic SInt32 StreamNOPWritePut(queue_t* q, mblk_t* mp)	// This routine is called by STREAMS when it has a message for our	// module from upstream.  Typically, this routine is a big case statement	// that dispatches to our various message handling routines.  However, the 	// function of this stream module is to pass through all messages unchanged,	// so the case statement is not very exciting.	//	// Environment: standard STREAMS entry point{	TRACE_SETUP;	PerStreamDataPtr streamData;		LOG_ENTRY( "StreamNOP:StreamNOPWritePut" );	OTAssert("StreamNOPWritePut: Not the write queue", IsWriteQ(q) );	// OTDebugBreak("StreamNOPWritePut: Entered");		streamData = GetPerStreamData(q);	switch ( GetPrimitive(mp) ) {		default:			putnext(q, mp);			break;	}		LOG_EXIT;		return 0;}/////////////////////////////////////////////////////////////////////// Read-side put routinestatic SInt32 StreamNOPReadPut(queue_t* q, mblk_t* mp)	// This routine is called by STREAMS when it has a message for our	// module from downstream.  Typically, this routine is a big case statement	// that dispatches to our various message handling routines.  However, the 	// function of this stream module is to pass through all messages unchanged,	// so the case statement is not very exciting.	//	// Environment: standard STREAMS entry point{	TRACE_SETUP;	PerStreamDataPtr streamData;		LOG_ENTRY( "StreamNOP:StreamNOPReadPut" );	OTAssert("StreamNOPReadPut: Not the read queue", IsReadQ(q) );	// OTDebugBreak("StreamNOPReadPut: Entered");		streamData = GetPerStreamData(q);	switch ( GetPrimitive(mp) ) {		default:			putnext(q, mp);			break;	}		LOG_EXIT;		return 0;}/////////////////////////////////////////////////////////////////////// Static Declaration Structuresstatic struct module_info gModuleInfo =  {	9992,						// Module Number, only useful for debugging	"StreamNOP",				// Name of module	0,							// Minimum data size	INFPSZ,						// Maximum data size	16384,						// Hi water mark for queue	4096						// Lo water mark for queue};static struct qinit gReadInit = {	StreamNOPReadPut,		// Put routine for "incoming" data	nil,						// Service routine for "incoming" data	StreamNOPOpen,			// Our open routine	StreamNOPClose, 		// Our close routine	nil,						// No admin routine	&gModuleInfo				// Our module_info};static struct qinit gWriteInit ={	StreamNOPWritePut,		// Put routine for client data	nil,						// Service routine for client data	nil,						// open  field only used in read-side structure	nil,						// close field only used in read-side structure	nil,						// admin field only used in read-side structure	&gModuleInfo				// Our module_info};static struct streamtab theStreamTab = {	&gReadInit,					// Our read-side qinit structure	&gWriteInit,				// Our write-side qinit structure	0,							// We are not a mux, so set this to nil	0							// We are not a mux, so set this to nil};/////////////////////////////////////////////////////////////////////// Macintosh-specific Static Structuresstatic struct install_info theInstallInfo ={	&theStreamTab,			// Stream Tab pointer	kOTModIsModule + kOTModUpperIsTPI + kOTModIsFilter,							// Tell OT that we are a driver, not a module	SQLVL_MODULE,			// Synchronization level, module level for the moment	0,						// Shared writer list buddy	0,						// Open Transport use - always set to 0	0						// Flag - always set to 0};// Prototypes for the exported routines below.extern Boolean InitStreamModule(void *portInfo);extern void TerminateStreamModule(void);extern install_info* GetOTInstallInfo();#pragma export list GetOTInstallInfo, InitStreamModule, TerminateStreamModule// Export entry pointextern Boolean InitStreamModule(void *portInfo)	// Initialises the module before the first stream is opened.	// Should return true if the module has started up correctly.	//	// Environment: Always called at SystemTask time.{		TRACE_SETUP;	#pragma unused(portInfo)	Boolean result;		OTDebugBreak("StreamNOP: InitStreamModule");		LOG_ENTRY( "StreamNOP:InitStreamModule" );	result = true;		LOG_EXIT;	return (result);}extern void TerminateStreamModule(void)	// Shuts down the module after the last stream has been	// closed.	//	// Environment: Always called at SystemTask time.{	TRACE_SETUP;		LOG_ENTRY( "StreamNOP:TerminateStreamModule" );		// It's an excellent idea to have the following in your code, just to make	// sure you haven't left any streams open before you quit.  In theory, OT	// should not call you until the last stream has been closed, but in practice	// this can happen if you use mi_detach to half-close a stream.		OTAssert("TerminateStreamModule: Streams are still active", gStreamList == nil);	LOG_EXIT;}extern install_info* GetOTInstallInfo()	// Return pointer to install_info to STREAMS.{	return &theInstallInfo;}