#include <al.h>
#include <alc.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <math.h>

bool populateBuffer(char* b, int samplePerBuffer, OggVorbis_File& oggFile) {
    int bytes(0);
    int totalBytes(0);
    int size(samplePerBuffer);
    int curr;
    do {
        bytes = ov_read(&oggFile, &(b[totalBytes]), size - totalBytes, 0, 2, 1, &curr);
        /*if(bytes == OV_HOLE) {
            puts("Error: ov_read - There was an interruption in the data.");
            break;
        } else if(bytes == OV_EBADLINK) {
            puts("Error: ov_read - an invalid stream section was supplied to libvorbisfile, or the requested link is corrupt.");
            break;
        } else if(bytes == OV_EINVAL) {
            puts("Error: ov_read - The initial file headers couldn't be read or are corrupt, or that the initial open call for vf failed.");
            break;
        } else {
            puts("A buffer was populated successfully!");
        }*/
        totalBytes += bytes;
    } while(bytes > 0 && size - totalBytes > 0);

    return bytes == 0;
}

int main(int argc, char* argv[]) {

    if(argc != 2) {
        puts("Usage: PlayOgg [Ogg Vorbis Filename]");
        exit(0);
    }

    // Display Default Device
    ALenum error(alGetError());
    puts("Default device:");
    printf("   %s\n\n", alcGetString(0, ALC_DEFAULT_DEVICE_SPECIFIER));
    error = alGetError();
    if(error == ALC_INVALID_ENUM) {
        //The specified parameter is not valid
        puts("Error: alcGetString - Invalid parameter.");
    }

    // Display List of Available Devices
    alGetError();
    const ALchar* deviceList(alcGetString(0, ALC_DEVICE_SPECIFIER));
    error = alGetError();
    if(error == ALC_INVALID_ENUM) {
        //The specified parameter is not valid
        puts("Error: alcGetString - Invalid parameter.");
    }
    puts("List of Available devices.");
    for(int i = 1; *deviceList; i++) {
        printf("%d. %s\n", i, deviceList);
        deviceList += std::string(deviceList).length() + 1;
    }

    puts("");

    // Choose Device
    //   we know there's only one on this system so we'll just stick with the default.
    //   this would not be proper for a product
    //const ALchar* deviceName(alcGetString(0, ALC_DEFAULT_DEVICE_SPECIFIER));
    ALCdevice* device = alcOpenDevice(0); // select the "preferred device"
    ALCcontext* context(0);
    if (device) {
        context = alcCreateContext(device, 0);
        alcMakeContextCurrent(context);
    }

    // Define Note Stream
    OggVorbis_File oggFile;
    printf("Attempting to open: %s\n", argv[1]);
    int ov_error(ov_fopen(argv[1], &oggFile));
    if(ov_error == OV_EREAD) {
        puts("Failure: ov_fopen - A read from media returned an error.");
    } else if(ov_error == OV_ENOTVORBIS) {
        puts("Failure: ov_fopen - Bitstream does not contain any Vorbis data.");
    } else if(ov_error == OV_EVERSION ) {
        puts("Failure: ov_fopen - Vorbis version mismatch.");
    } else if(ov_error == OV_EBADHEADER ) {
        puts("Failure: ov_fopen - Invalid Vorbis bitstream header.");
    } else if(ov_error == OV_EFAULT ) {
        puts("Failure: ov_fopen - Internal logic fault; indicates a bug or heap/stack corruption.");
    } else if(ov_error < 0) {
        puts("Failure: ov_fopen - Failed somewhere... dunno where this error isn't covered in the documentation...");
    }else {
        puts("Ogg Vorbis file opened successfully!");
    }

    // Generate Buffers
    ALsizei bufferCount(2);
    ALuint buffers[bufferCount];
    alGetError();
    alGenBuffers(bufferCount, buffers);
    error = alGetError();
    if(error == AL_INVALID_VALUE) {
        // The buffer array isn't large enough to hold the number of buffers requested.
        puts("Error: alGenBuffers - The buffer array isn't large enough to hold the number of buffers requested.");
    } else if(error == AL_OUT_OF_MEMORY) {
        // There is not enough memory available to generate all the buffers requested.
        puts("Error: alGenBuffers - There is not enough memory available to generate all the buffers requested.");
    } else {
        puts("Buffer generated sucessfully!");
    }

    // Allocate buffer memory
    int samplePerBuffer(24000);
    int sampleRate(44100);
    char buffer1[samplePerBuffer];
    char buffer2[samplePerBuffer];
    // Load initial data into buffers
    populateBuffer(buffer1, samplePerBuffer, oggFile);
    populateBuffer(buffer2, samplePerBuffer, oggFile);
    // Attach buffers to buffers...
    alBufferData(buffers[0], AL_FORMAT_STEREO16, buffer1, samplePerBuffer, sampleRate);
    alBufferData(buffers[1], AL_FORMAT_STEREO16, buffer2, samplePerBuffer, sampleRate);

    // Generate Sources
    alGetError();
    ALuint sources;
    alGenSources((ALsizei)1, &sources);
    error = alGetError();
    if(error == AL_OUT_OF_MEMORY) {
        // There is not enough memory to generate all the requested sources.
        puts("Error: alGenSources - There is not enough memory to generate all the requested sources.");
    } else if(error == AL_INVALID_VALUE) {
        // There are not enough non-memory resources to create all the requested sources, or the array pointer is not valid.
        puts("Error: alGenSources - There are not enough non-memory resources to create all the requested sources, or the array pointer is not valid.");
    } else if(error == AL_INVALID_OPERATION) {
        // There is no context to create sources in.
        puts("Error: alGenSources - There is no context to create sources in.");
    } else {
        puts("Sources generated successfully!");
    }

    // Attach Buffers
    // Queue all buffers
    alGetError();
    alSourceQueueBuffers(sources, bufferCount, (ALuint*)buffers);
    error = alGetError();
    if(error == AL_INVALID_NAME) {
        // At least one specified buffer name is not valid, or the specified source name is not valid.
        puts("Error: alSourceQueueBuffers - At least one specified buffer name is not valid, or the specified source name is not valid.");
    } else if(error == AL_INVALID_OPERATION) {
        // There is no current context, an attempt was made to add a new buffer which is not the same format as the buffers already in the queue, or the source already has a static buffer attached.
        puts("Error: alSourceQueueBuffers - There is no current context, an attempt was made to add a new buffer which is not the same format as the buffers already in the queue, or the source already has a static buffer attached.");
    }

    // Set Position
    alGetError();
    alSource3i(sources, AL_POSITION, 0, 0, 0);
    error = alGetError();
    if(error == AL_INVALID_VALUE) {
        // The value given is out of range.
        puts("Error: alSourcei - The value given is out of range.");
    } else if(error == AL_INVALID_ENUM) {
        // The specified parameter is not valid.
        puts("Error: alSourcei - The specified parameter is not valid.");
    } else if(error == AL_INVALID_NAME) {
        // The specified source name is not valid.
        puts("Error: alSourcei - The specified source name is not valid.");
    } else if(error == AL_INVALID_OPERATION) {
        // There is no current context.
        puts("Error: alSourcei - There is no current context.");
    } else {
        puts("Source position set successfully!");
    }

    // Play Audio
    alGetError();
    alSourcePlay(sources);
    error == alGetError();
    if(error == AL_INVALID_NAME) {
        // The specified source name is not valid.
        puts("Error: alSourcePlay - The specified source name is not valid.");
    } else if(error == AL_INVALID_OPERATION) {
        // There is no current context.
        puts("Error: alSourcePlay - There is no current context.");
    } else {
        puts("Playing...");
    }

    //Play audio
    bool playing(true);
    bool flag(false);
    bool eof(false);
    while(playing) {

        // If buffer done playing them reuse
        ALint value;
        alGetSourceiv(sources, AL_BUFFERS_PROCESSED, &value);
        if(value) {
            if(!eof) {
                if(flag) {
                    alSourceUnqueueBuffers(sources, 1, &buffers[1]);
                    eof = populateBuffer(buffer2, samplePerBuffer, oggFile);
                    alBufferData(buffers[1], AL_FORMAT_STEREO16, buffer1, samplePerBuffer, sampleRate);
                    alSourceQueueBuffers(sources, 1, &buffers[1]);
                } else {
                    alSourceUnqueueBuffers(sources, 1, &buffers[0]);
                    eof = populateBuffer(buffer1, samplePerBuffer, oggFile);
                    alBufferData(buffers[0], AL_FORMAT_STEREO16, buffer1, samplePerBuffer, sampleRate);
                    alSourceQueueBuffers(sources, 1, &buffers[0]);
                }
                flag != flag;
            }
        }

        // If no more data to be played then set playing to false
        alGetSourcei(sources, AL_SOURCE_STATE, &value);
        if(value == AL_STOPPED) {
            playing = false;
        }

    }

    puts("Done playing.");

    // Close ogg file
    ov_error = ov_clear(&oggFile);
    if(ov_error) {
        puts("Failure: ov_clear - Failed.");
    }

    // Stop Audio
    alGetError();
    alSourceStop(sources);
    error == alGetError();
    if(error == AL_INVALID_NAME) {
        // The specified source name is not valid.
        puts("Error: alSourcePlay - The specified source name is not valid.");
    } else if(error == AL_INVALID_OPERATION) {
        // There is no current context.
        puts("Error: alSourcePlay - There is no current context.");
    } else {
        puts("Source stopped successfully!");
    }

    // Detach Buffers
    alGetError();
    alSourcei(sources, AL_BUFFER, 0);
    error = alGetError();
    if(error == AL_INVALID_VALUE) {
        // The value given is out of range.
        puts("Error: alSourcei - The value given is out of range.");
    } else if(error == AL_INVALID_ENUM) {
        // The specified parameter is not valid.
        puts("Error: alSourcei - The specified parameter is not valid.");
    } else if(error == AL_INVALID_NAME) {
        // The specified source name is not valid.
        puts("Error: alSourcei - The specified source name is not valid.");
    } else if(error == AL_INVALID_OPERATION) {
        // There is no current context.
        puts("Error: alSourcei - There is no current context. ");
    } else {
        puts("Buffer detached successfully!");
    }

    // Delete Sources
    alGetError();
    alDeleteSources((ALuint)1, &sources);
    error = alGetError();
    if(error == AL_INVALID_NAME) {
        // At least one specified source is not valid, or an attempt is being made to delete more sources than exist.
        puts("Error: alDeleteSources - At least one specified source is not valid, or an attempt is being made to delete more sources than exist.");
    } else if(error == AL_INVALID_OPERATION) {
        // There is no current context.
        puts("Error: alDeleteSources - There is no current context.");
    } else {
        puts("Sources deleted successfully!");
    }

    // Delete Buffers
    //   must be done after buffers are detached from sources.
    alGetError();
    alDeleteBuffers(bufferCount, buffers);
    error = alGetError();
    if(error == AL_INVALID_OPERATION) {
        // The buffer is still in use and cannot be delete.
        puts("Error: alDeleteBuffers - The buffer is still in use and cannot be delete.");
    } else if(error == AL_INVALID_NAME) {
        // A buffer name is invalid.
        puts("Error: alDeleteBuffers - A buffer name is invalid.");
    } else if(error == AL_INVALID_VALUE) {
        // The requested number of buffers cannot be deleted.
        puts("Error: alDeleteBuffers - The requested number of buffers cannot be deleted.");
    } else {
        puts("Buffers deleted successfully!");
    }

    // Close Device
    context = alcGetCurrentContext();
    device = alcGetContextsDevice(context);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alGetError();
    ALCboolean success(alcCloseDevice(device));
    error = alGetError();
    if(error == ALC_INVALID_DEVICE) {
        // The specified device name doesn't exist.
        puts("Error: alcCloseDevice - Device name doesn't exist.");
    } else if(!success) {
        // Closing the device failed (device contains contexts or buffers).
        puts("Failure: alcCloseDevice - Device contains contexts or buffers; device cannot close.");
    } else {
        puts("Device closed successfully!");
    }

    puts("Terminating Program - Good-bye!");

    return 0;
}
