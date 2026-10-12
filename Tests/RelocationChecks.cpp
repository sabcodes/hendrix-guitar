#include <AudioToolbox/AudioToolbox.h>
#include <dlfcn.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
static void check(OSStatus status,const char* operation) {
 if(status!=noErr){std::fprintf(stderr,"%s failed: %d\n",operation,int(status));std::exit(1);}
}
int main(int argc,char** argv) {
 if(argc!=2)return 1;
 void* binary=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
 if(!binary){std::fprintf(stderr,"%s\n",dlerror());return 1;}
 auto factory=reinterpret_cast<AudioComponentFactoryFunction>(dlsym(binary,"JuceAUFactory"));
 if(!factory){std::fprintf(stderr,"AU factory missing\n");return 1;}
 AudioComponentDescription description{kAudioUnitType_MusicDevice,'Hdg1','Hdxg',0,0};
 auto component=AudioComponentRegister(&description,CFSTR("Hendrix Relocation Check"),0x00040000,factory);
 if(!component)return 1;
 AudioUnit unit=nullptr;check(AudioComponentInstanceNew(component,&unit),"create");
 AudioStreamBasicDescription format{48000,kAudioFormatLinearPCM,kAudioFormatFlagIsFloat|kAudioFormatFlagIsPacked|kAudioFormatFlagIsNonInterleaved,4,1,4,2,32,0};
 check(AudioUnitSetProperty(unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,0,&format,sizeof(format)),"format");
 UInt32 capacity=256;check(AudioUnitSetProperty(unit,kAudioUnitProperty_MaximumFramesPerSlice,kAudioUnitScope_Global,0,&capacity,sizeof(capacity)),"block size");
 check(AudioUnitInitialize(unit),"initialise");
 check(MusicDeviceMIDIEvent(unit,0x90,57,100,0),"note on");
 float samples[2][256]{};struct Buffers {UInt32 count;AudioBuffer channel[2];} buffers{2,{{1,sizeof(samples[0]),samples[0]},{1,sizeof(samples[1]),samples[1]}}};
 float peak=0;
 for(int block=0;block<64;++block) {
  AudioTimeStamp stamp{};stamp.mFlags=kAudioTimeStampSampleTimeValid;stamp.mSampleTime=block*256;
  AudioUnitRenderActionFlags flags=0;check(AudioUnitRender(unit,&flags,&stamp,0,256,reinterpret_cast<AudioBufferList*>(&buffers)),"render");
  for(auto& channel:samples)for(float sample:channel){if(!std::isfinite(sample)||std::abs(sample)>1)return 1;peak=std::fmax(peak,std::abs(sample));}
 }
 check(AudioUnitReset(unit,kAudioUnitScope_Global,0),"host reset");
 AudioTimeStamp resetStamp{};resetStamp.mFlags=kAudioTimeStampSampleTimeValid;resetStamp.mSampleTime=64*256;
 AudioUnitRenderActionFlags resetFlags=0;check(AudioUnitRender(unit,&resetFlags,&resetStamp,0,256,reinterpret_cast<AudioBufferList*>(&buffers)),"render after reset");
 for(auto& channel:samples)for(float sample:channel)if(sample!=0){std::fprintf(stderr,"Host reset left sound active\n");return 1;}
 check(MusicDeviceMIDIEvent(unit,0x80,57,0,0),"note off");
 check(AudioUnitUninitialize(unit),"uninitialise");check(AudioComponentInstanceDispose(unit),"dispose");
 std::printf("Relocated AU renders bundled guitar; peak %.6f\n",peak);
 return peak>.0001f?0:1;
}
