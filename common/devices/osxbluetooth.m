/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2015       Fabrizio Gennari

*/

#include "config.h"

#ifdef HAVE_BLUETOOTH_MACOSX

#include <IOBluetooth/objc/IOBluetoothRFCOMMChannel.h>
#include <IOBluetooth/objc/IOBluetoothDevice.h>

#include "devices/bluetooth.h"

static NSMutableDictionary *queues;
static int next_fd = 1;

@interface GnokiiOSXBluetooth : NSObject <IOBluetoothRFCOMMChannelDelegate> {
@private
    NSMutableData *queue;
    IOBluetoothRFCOMMChannel *channel;
}
- (BOOL)connect:(const char*)addr chid:(BluetoothRFCOMMChannelID)channelID;
- (int)select:(CFAbsoluteTime)timeoutDate;
- (int)read:(__ptr_t)bytes size:(int)size;
- (BOOL)write:(const __ptr_t *)data length:(int)length;
@end

@implementation GnokiiOSXBluetooth
- (id)init {
    if ((self = [super init])) {
        queue = [[NSMutableData alloc] init];
    }
    return self;
}

- (BOOL)connect:(const char*)addr chid:(BluetoothRFCOMMChannelID)channelID {
    NSString *address = [[[NSString alloc] initWithCString:addr encoding:NSASCIIStringEncoding] autorelease];
    IOBluetoothDevice* btDevice = [[IOBluetoothDevice deviceWithAddressString:address] autorelease];
    return btDevice != nil &&
    [btDevice openRFCOMMChannelSync:&channel withChannelID:channelID delegate:self] == kIOReturnSuccess; // after connection it is established.. the delegates methoed are triggered.
}

-(BOOL)write:(const __ptr_t *)data length:(int)length {
    int i;\
    IOReturn ret = [channel writeSync:(void*)data length:(UInt16)length];
    return ret == kIOReturnSuccess;
}

- (void)dealloc {
    if (channel != nil) {
        [channel closeChannel];
        [[channel getDevice] closeConnection];
        [channel release];
    }
    [queue release];
    [super dealloc];
}

- (int)waitForData:(NSTimeInterval)timeout {
    int ret = -1;
    if ([queue length] <= 0 && [channel isOpen]) {
        NSTimer *timer = nil;
        if (timeout > 0){
            timer =     [NSTimer scheduledTimerWithTimeInterval:timeout
             target:self
             selector:@selector(timerElapsed:)
             userInfo:nil
             repeats:NO];
        }
        CFRunLoopRun();
        if (timer != nil){
            if ([timer isValid])
                [timer invalidate];
            else
                ret = 0;
        }
    }
    if ([queue length] > 0)
        return 1;
    return ret;
}

- (int)select:(NSTimeInterval)timeout {
    return [self waitForData:timeout];
}

-(int)read:(__ptr_t)bytes size:(int)size {
    if (![self waitForData:0])
        return -1;

    if (size > [queue length])
        size = [queue length];
    memcpy (bytes, [queue mutableBytes], size);
    NSRange range = {0, size};
    [queue replaceBytesInRange:range withBytes:nil length:0];
    return size;
}

- (void)rfcommChannelData:(IOBluetoothRFCOMMChannel*)rfcommChannel data:(void *)dataPointer length:(size_t)dataLength {
    int i;
    [queue appendBytes:dataPointer length:dataLength];
    if ([queue length] > 0)
        CFRunLoopStop(CFRunLoopGetCurrent());
}

- (void)rfcommChannelClosed:(IOBluetoothRFCOMMChannel*)rfcommChannel {
    if (channel == rfcommChannel)
        CFRunLoopStop(CFRunLoopGetCurrent());
}
- (void) timerElapsed:(NSTimer*) timer {
    CFRunLoopStop(CFRunLoopGetCurrent());
}
@end

int bluetooth_open(const char* addr, uint8_t channel_num, struct gn_statemachine* state)
{
    if (queues == nil)
        queues = [NSMutableDictionary dictionaryWithCapacity:1];
    GnokiiOSXBluetooth *q = [[GnokiiOSXBluetooth alloc] init];
    if (![q connect:addr chid:(BluetoothRFCOMMChannelID)channel_num]) { // after connection it is established.. the delegates methoed are triggered.
        [q release];
        return -1;
    }
    int ret = next_fd++;
    [queues setObject:q forKey:@(ret)];
    return ret;
}

int bluetooth_write(int fd, const __ptr_t bytes, int size, struct gn_statemachine *state)
{
    if (queues == nil)
        return -1;
    GnokiiOSXBluetooth *q = [queues objectForKey:@(fd)];
    sleep(2);
    if (q == nil
        || ![q write:bytes length:size])
        return -1;

    return size;
}

int bluetooth_read(int fd, __ptr_t bytes, int size, struct gn_statemachine *state)
{
    if (queues == nil)
        return -1;
    GnokiiOSXBluetooth *q = [queues objectForKey:@(fd)];
    if (q == nil)
        return -1;
    return [q read:bytes size:size];
}

int bluetooth_select(int fd, struct timeval *timeout, struct gn_statemachine *state)
{
    if (queues == nil)
        return -1;
    GnokiiOSXBluetooth *q = [queues objectForKey:@(fd)];
    if (q == nil)
        return -1;

    return [q select:(NSTimeInterval)(timeout->tv_sec + timeout->tv_usec / 1000000.0)];
}

int bluetooth_close(int fd, struct gn_statemachine *state)
{
    if (queues == nil)
        return -1;
    GnokiiOSXBluetooth *q = [queues objectForKey:@(fd)];
    if (q == nil)
        return -1;
    [q release];
    [queues removeObjectForKey:@(fd)];
    return 1;
}

#endif
