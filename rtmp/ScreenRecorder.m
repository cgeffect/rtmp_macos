//
//#import "ScreenRecorder.h"
//
//@implementation ScreenRecorder
//
//- (instancetype)init {
//    self = [super init];
//    if (self) {
//        self.captureSession = [[AVCaptureSession alloc] init];
//        self.movieFileOutput = [[AVCaptureMovieFileOutput alloc] init];
//        [self.captureSession addOutput:self.movieFileOutput];
//    }
//    return self;
//}
//
//- (void)startCapture {
//    NSError *error = nil;
//    SCShareableContent *content = [SCShareableContent shareableContentExcludingDesktopWindows:NO onScreenWindowsOnly:YES error:&error];
//    if (error) {
//        NSLog(@"Error getting shareable content: %@", error);
//        return;
//    }
//
//    SCContentFilter *filter = [SCContentFilter filterWithDisplay:content.displays.firstObject excludingApplications:@[] exceptingWindows:@[]];
//
//    SCStreamConfiguration *config = [[SCStreamConfiguration alloc] init];
//    config.capturesAudio = YES;
//    config.excludesCurrentProcessAudio = NO;
//    config.width = content.displays.firstObject.width;
//    config.height = content.displays.firstObject.height;
//    config.minimumFrameInterval = CMTimeMake(1, 30); // 30 FPS
//
//    self.stream = [[SCStream alloc] initWithContentFilter:filter configuration:config delegate:self];
//    [self.stream start];
//
//    // Start video recording
//    AVCaptureDevice *videoDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
//    AVCaptureDeviceInput *videoInput = [AVCaptureDeviceInput deviceInputWithDevice:videoDevice error:&error];
//    if (error) {
//        NSLog(@"Error creating video input: %@", error);
//        return;
//    }
//    [self.captureSession addInput:videoInput];
//
//    AVCaptureDevice *audioDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];
//    AVCaptureDeviceInput *audioInput = [AVCaptureDeviceInput deviceInputWithDevice:audioDevice error:&error];
//    if (error) {
//        NSLog(@"Error creating audio input: %@", error);
//        return;
//    }
//    [self.captureSession addInput:audioInput];
//
//    [self.captureSession startRunning];
//
//    NSURL *outputURL = [NSURL fileURLWithPath:@"/path/to/output/file.mp4"];
//    [self.movieFileOutput startRecordingToOutputFileURL:outputURL recordingDelegate:self];
//}
//
//- (void)stopCapture {
//    // [self.stream stop];
//    // [self.captureSession stopRunning];
//    // [self.movieFileOutput stopRecording];
//}
//
//- (void)stream:(SCStream *)stream didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer ofType:(SCStreamOutputType)type {
//    if (type == SCStreamOutputTypeScreen) {
//        // Handle video sample buffer
//    } else if (type == SCStreamOutputTypeAudio) {
//        // Handle audio sample buffer
//    }
//}
//
//@end
