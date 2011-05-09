#include <v8.h>
#include <node.h>
#include "SDL.h"
#include "SDL_mixer.h"

using namespace v8;
using namespace node;

class SDL : public ObjectWrap {
    static Persistent<FunctionTemplate> constructor_template;

public:
    //Async(int xx, int yy) : x(xx), y(yy) {}

    static void Initialize(Handle<Object> target) {
        HandleScope scope;

        Local<FunctionTemplate> t = FunctionTemplate::New(New);
        constructor_template = Persistent<FunctionTemplate>::New(t);
        constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
        constructor_template->SetClassName(String::NewSymbol("SDL"));

        NODE_SET_PROTOTYPE_METHOD(constructor_template, "play", Play);
        target->Set(String::NewSymbol("SDL"), constructor_template->GetFunction());
        
        /* Initialize the SDL library */
        if ( SDL_Init(SDL_INIT_AUDIO) < 0 ) {
          fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
          return ;
        }

        int audio_rate;
        Uint16 audio_format;
        int audio_channels;

        /* Initialize variables */
        audio_rate = 44100;
        audio_format = AUDIO_S16SYS;
        audio_channels = 2;

        /* Open the audio device */
        if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, 4096) < 0) {
          fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
        }
        else {
          Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
          printf("Opened audio at %d Hz %d bit %s\n", audio_rate,
      			(audio_format&0xFF),
      			(audio_channels > 2) ? "surround" :
      			(audio_channels > 1) ? "stereo" : "mono");

      }
    }

    static Handle<Value> New(const Arguments &args) {
        HandleScope scope;

        SDL *sdl = new SDL();
        sdl->Wrap(args.This());
        return args.This();
    }

    struct playInfo {
        Persistent<Function> cb;
        SDL *sdl;
        const char * fileName;
    };

    static int EIO_Play(eio_req *req) {
        playInfo *info = (playInfo *)req->data;
        SDL *sdl = info->sdl;
        //const char fileName = info->fileName;
        Mix_Chunk *wave = NULL;
        int channel;
        printf("Loading %s\n", info->fileName);
        wave = Mix_LoadWAV(info->fileName);

        printf("Playing\n");
        /* Play and then exit */
        channel = Mix_PlayChannel(-1, wave, 0);

        while (Mix_Playing(channel)) {
          SDL_Delay(1);
        }
        Mix_FreeChunk(wave);
        Mix_CloseAudio();
        return 0;
    }

    static int EIO_PlayAfter(eio_req *req) {
        HandleScope scope;

        ev_unref(EV_DEFAULT_UC);
        playInfo *info = (playInfo *)req->data;
        Local<Value> argv[1];

        TryCatch try_catch;

        info->cb->Call(Context::GetCurrent()->Global(), 1, argv);

        if (try_catch.HasCaught())
            FatalException(try_catch);

        info->cb.Dispose();

        info->sdl->Unref();
        free(info);

        return 0;
    }

    static Handle<Value> Play(const Arguments &args) {
        HandleScope scope;
        
        if (args.Length() < 1) {
          return ThrowException(Exception::TypeError(String::New("Bad argument")));
        }
        Local<Function> cb = Local<Function>::Cast(args[1]);
        SDL *sdl = ObjectWrap::Unwrap<SDL>(args.This());
        
        String::AsciiValue fileName(args[0]->ToString());
        playInfo *info = (playInfo *)malloc(sizeof(playInfo));
        info->cb = Persistent<Function>::New(cb);
        info->sdl = sdl;
        info->fileName = *fileName;

        eio_custom(EIO_Play, EIO_PRI_DEFAULT, EIO_PlayAfter, info);

        ev_ref(EV_DEFAULT_UC);
        sdl->Ref();

        return Undefined();
    }
};

Persistent<FunctionTemplate> SDL::constructor_template;

extern "C" void
init(Handle<Object> target)
{
    HandleScope scope;

    SDL::Initialize(target);
}

/*
#define NODE_SET_METHOD(obj, name, callback)                              \
  obj->Set(v8::String::NewSymbol(name),                                   \
           v8::FunctionTemplate::New(callback)->GetFunction())

static int still_playing(void)
{
  return(Mix_Playing(0));
}

static Handle<Value> Play(const Arguments& args) {
  HandleScope scope;
  
  Mix_Chunk *wave = NULL;

  if (args.Length() < 1) {
    return ThrowException(Exception::TypeError(String::New("Bad argument")));
  }


  String::AsciiValue fileName(args[0]->ToString());
  printf("Loading %s\n",*fileName);
  wave = Mix_LoadWAV(*fileName);

  printf("Playing\n");

  Mix_PlayChannel(0, wave, 0);

  while (still_playing()) {
    SDL_Delay(1);

  } 
  printf("Done\n");
  return scope.Close(args[0]);
}


extern "C" void
init (Handle<Object> target)
{
  HandleScope scope;
  target->Set(String::New("hello"), String::New("world"));
  NODE_SET_METHOD(target, "play", Play);


  if ( SDL_Init(SDL_INIT_AUDIO) < 0 ) {
    fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
    return ;
  }

  int audio_rate;
  Uint16 audio_format;
  int audio_channels;


  audio_rate = MIX_DEFAULT_FREQUENCY;
  audio_format = MIX_DEFAULT_FORMAT;
  audio_channels = 2;


  if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, 4096) < 0) {
    fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
  }
  else {
    Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
    printf("Opened audio at %d Hz %d bit %s\n", audio_rate,
			(audio_format&0xFF),
			(audio_channels > 2) ? "surround" :
			(audio_channels > 1) ? "stereo" : "mono");

    }
}
*/