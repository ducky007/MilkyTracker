/*
 *  milkyplay/drivers/generic/RtAudio3Impl.cpp
 *
 *  Copyright 2008 Peter Barth
 *
 *  This file is part of Milkytracker.
 *
 *  Milkytracker is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Milkytracker is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Milkytracker.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 *  RtAudio3Impl.cpp
 *  MilkyPlay
 *
 *  Created by Peter Barth on 06.04.08.
 *
 *
 */

#include "AudioDriver_RTAUDIO.h"
#include "RtAudio3.h"

#ifdef DRIVER_UNIX
#include <sys/types.h>
#include <unistd.h> 
#endif

static const char*driverNames[] =
{
	"Unspecified (RtAudio)",
	"Alsa (RtAudio)",
	"OSS (RtAudio)",
	"Jack (RtAudio)",
	"CoreAudio (RtAudio)",
	"Irix (RtAudio)",
	"ASIO (RtAudio)",
	"DirectSound (RtAudio)"
};

class AudioDriverImpl : public AudioDriver_COMPENSATE
{
private:
	RtAudio* audio;
	RtAudio::RtAudioApi selectedAudioApi;
	
	static int fill_audio(char* stream, int length, void *udata)
	{
		// upgrade to reflect number of bytes, instead number of samples
		length<<=2;
		
		AudioDriverImpl* audioDriver = (AudioDriverImpl*)udata;
		
		// Base class can handle this
		audioDriver->fillAudioWithCompensation((char *) stream, length);
		return 0;
	}
	
public:
	AudioDriverImpl(RtAudio::RtAudioApi audioApi = RtAudio::UNSPECIFIED) :
		AudioDriver_COMPENSATE(),
		audio(NULL),
		selectedAudioApi(audioApi)
	{
	}

	virtual	~AudioDriverImpl()
	{
	}
			
	virtual mp_sint32 getPreferredBufferSize() const
	{ 
		switch (selectedAudioApi)
		{
			case RtAudio::UNSPECIFIED:
				return 1024;
			case RtAudio::LINUX_ALSA:
				return 2048;
			case RtAudio::LINUX_OSS:
				return 2048;
			case RtAudio::MACOSX_CORE:
				return 1024;
			case RtAudio::WINDOWS_ASIO:
				return 1024;
			case RtAudio::WINDOWS_DS:
				return 2048;
		}
		
		return 2048;
	}	

	// On error return a negative value
	// If the requested buffer size can be served return 0, 
	// otherwise return the number of 16 bit words contained in the obtained buffer
	virtual mp_sint32 initDevice(mp_sint32 bufferSizeInWords, mp_uint32 mixFrequency, MasterMixer* mixer)
	{
		/*mp_sint32 res = AudioDriverBase::initDevice(bufferSizeInWords, mixFrequency, mixer);
		if (res < 0)
			return res;

		// construction will not throw any RtAudio specific exceptions
		audio = new RtAudio(selectedAudioApi);
		
		mp_uint32 numDevices = audio->getDeviceCount();
		
		mp_uint32 deviceId = 0;
		for (mp_uint32 i = 0; i < numDevices; i++)
		{
			RtAudio::DeviceInfo deviceInfo = audio->getDeviceInfo(i);
			if (deviceInfo.isDefaultOutput)
			{
				deviceId = i;
				break;
			}
		}
		

	#ifdef DRIVER_UNIX
		int channels = 2;
		unsigned int sampleRate = mixFrequency;
		unsigned int bufferSize = bufferSizeInWords / channels;
		RtAudio::StreamParameters sStreamParams;
		sStreamParams.deviceId = deviceId;
		RtAudio::StreamOptions sStreamOptions;
		char streamName[32];
		snprintf(streamName, sizeof(streamName), "Milkytracker %i", getpid());
		sStreamOptions.streamName = streamName;
		sStreamOptions.numberOfBuffers = 2;
		sStreamParams.nChannels = 2;
		
		// Open a stream during RtAudio instantiation
		try 
		{
			audio->openStream(&sStreamParams, NULL, RTAUDIO_SINT16,
								sampleRate, &bufferSize, &fill_audio, (void *)this,
								&sStreamOptions);
		}
	#else
		int channels = 2;
		unsigned int sampleRate = mixFrequency;
		unsigned int bufferSize = bufferSizeInWords / channels;
		RtAudio::StreamParameters sStreamParams;
		sStreamParams.deviceId = deviceId;
		sStreamParams.nChannels = 2;
		
		// Open a stream during RtAudio instantiation
		try 
		{
			audio->openStream(&sStreamParams, NULL, RTAUDIO_SINT16,
								sampleRate, &bufferSize, &fill_audio, (void *)this);
		}
	#endif
		catch (RtError &error) 
		{
			error.printMessage();
			return -1;
		}
		
	#ifndef WIN32
		printf("Audio buffer: Wanted %d bytes, got %d\n", bufferSizeInWords / channels * 4, bufferSize * 4);
	#endif

		// If we got what we requested, return 0,
		// otherwise return the actual number of samples * number of channels
		return (bufferSizeInWords / channels == (signed)bufferSize) ? 0 : bufferSize * channels;*/
		
		mp_sint32 res = AudioDriverBase::initDevice(bufferSizeInWords, mixFrequency, mixer);
		if (res < 0)
			return res;

		int channels = 2;
		int sampleRate = mixFrequency;
		int bufferSize = bufferSizeInWords / channels;
		int nBuffers = 4;      
		int device = 0;        // 0 indicates the default or first available device
		
		// Open a stream during RtAudio instantiation
		try 
		{
			audio = new RtAudio(device, channels, 0, 0, RTAUDIO_SINT16,
								sampleRate, &bufferSize, nBuffers, selectedAudioApi);
								
			audio->setStreamCallback(&fill_audio, (void *)this);
		}
		catch (RtError &error) 
		{
			error.printMessage();
			return -1;
		}
		
	#ifndef WIN32
		printf("Audio buffer: Wanted %d bytes, got %d\n", bufferSizeInWords / channels * 4, bufferSize * 4);
	#endif

		// If we got what we requested, return 0,
		// otherwise return the actual number of samples * number of channels
		return (bufferSizeInWords / channels == bufferSize) ? 0 : bufferSize * channels;		
	}

	virtual mp_sint32 stop()
	{
		if (audio)
		{
			try 
			{
				audio->stopStream();
				deviceHasStarted = false;
				return 0;
			}
			catch (RtError &error) 
			{
				error.printMessage();
				return -1;
			}
		}
		else return -1;
	}

	virtual mp_sint32 closeDevice()
	{	
		if (audio)
		{
			try 
			{
				audio->closeStream();
				deviceHasStarted = false;
				return 0;
			}
			catch (RtError &error) 
			{
				error.printMessage();
				return -1;
			}
		}
		else return -1;
	}

	virtual void start()
	{
		if (audio)
		{
			try 
			{
				audio->startStream();
				deviceHasStarted = true;
			}
			catch (RtError &error) 
			{
				error.printMessage();
			}			
		}
	}

	virtual mp_sint32 pause()
	{
		if (audio)
		{
			try 
			{
				audio->stopStream();
				return 0;
			}
			catch (RtError &error) 
			{
				error.printMessage();		
				return -1;
			}
		}
		else return -1;
	}

	virtual mp_sint32 resume()
	{
		if (audio)
		{
			try 
			{
				audio->startStream();
				return 0;
			}
			catch (RtError &error) 
			{
				error.printMessage();		
				return -1;
			}
		}
		else return -1;
	}
		
	virtual		const char* getDriverID() { return driverNames[selectedAudioApi]; }
};

void AudioDriver_RTAUDIO::createRt3Instance(Api audioApi/* = UNSPECIFIED*/)
{
	RtAudio::RtAudioApi rtApi = RtAudio::UNSPECIFIED;
	switch (audioApi)
	{
		case LINUX_ALSA:
			rtApi = RtAudio::LINUX_ALSA;
			break;
		case LINUX_OSS:
			rtApi = RtAudio::LINUX_OSS;
			break;
		case MACOSX_CORE:
			rtApi = RtAudio::MACOSX_CORE;
			break;
		case WINDOWS_ASIO:
			rtApi = RtAudio::WINDOWS_ASIO;
			break;
		case WINDOWS_DS:
			rtApi = RtAudio::WINDOWS_DS;
			break;
	}
	if (impl)
		delete impl;
	impl = new AudioDriverImpl(rtApi);
}

