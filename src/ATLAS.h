

#if defined(PBL_PLATFORM_EMERY)
    #define XCENTRE 100
    #define YCENTRE 114
    #define X (XCENTRE-125)
    #define Y (YCENTRE-125)
    #define CLOCKNUDGE 26
#elif defined(PBL_PLATFORM_GABBRO)
    #define XCENTRE 130
    #define YCENTRE 130
    #define X 0
    #define Y 0
    #define CLOCKNUDGE 26
#elif defined(PBL_RECT)
    #define XCENTRE 72
    #define YCENTRE 84
    #define X (XCENTRE-90)
    #define Y (YCENTRE-90)
    #define CLOCKNUDGE 0
#else
    // Chalk
    #define XCENTRE 90
    #define YCENTRE 90
    #define X 0
    #define Y 0
    #define CLOCKNUDGE 18
#endif

#if defined(PBL_PLATFORM_EMERY) 
  #define HR 1.388f 
#elif defined(PBL_PLATFORM_GABBRO)
  #define HR 1.444f 
#else
  #define HR 1
#endif

#define WIDTH (XCENTRE*2)
#define HEIGHT (YCENTRE*2)

#define ID_END (30*HR)
#define EM_END (48*HR)
#define HCAL_END (66*HR)

#define N_EMHAD 2
#define N_DECAY 4

#define TEXTSIZE 12

void updateWeather();
void tickHandler(struct tm* tickTime, TimeUnits unitsChanged);

Layer* getLayer();
