#import "ViewController.h"
#include <string>

@interface ViewController ()
{
}
@end

@implementation ViewController

- (instancetype)init {
    self = [super init];
    if (self) {
        NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 1000, 600)];
        view.wantsLayer = YES;
        view.layer.backgroundColor = [NSColor whiteColor].CGColor;
        self.view = view;
    }
    return self;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // FILE *file = fopen("/Users/jason/Desktop/yuv_pcm/yuv420p_720x1280.yuv", "rb");
    // uint8_t *data = (uint8_t *)malloc(720 * 1280 * 3 / 2);
    // live::H265Encoder encoder;
    // encoder.init(720, 1280, 30, 720*1280*4);
    // int index = 0;
    // while (true) {
    //     size_t size = fread(data, 1, 720 * 1280 * 3 / 2, file);
    //     if (size <= 0) {
    //         break;
    //     }
    //     encoder.encodeI420(data);
    //     printf("index: %d\n", index);
    //     index++;
    // }
    
    // encoder.flush();
}

@end

// 手动把这些配置放到plist文件里面 程序需要被编译为MACOS_BUNDLE
// <key>NSCameraUsageDescription</key>
// <string>Your app needs access to the camera to record videos.</string>
// <key>NSMicrophoneUsageDescription</key>
// <string>Your app needs access to the microphone to record audio.</string>
// <key>NSAppleEventsUsageDescription</key>
// <string>Your app needs access to send Apple events.</string>
